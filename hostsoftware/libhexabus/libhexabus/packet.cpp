#include "packet.hpp"

#include "common.hpp"
#include "crc.hpp"

#include <string.h>
#include <netinet/in.h>

#include <iostream>
#include <cstdio>
#include <errno.h>

#pragma GCC diagnostic warning "-Wstrict-aliasing"

using namespace hexabus;

hxb_packet_query Packet::query(uint32_t eid, bool ep_query)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_query packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = ep_query ? HXB_PTYPE_EPQUERY : HXB_PTYPE_QUERY;
  packet.flags = 0;
  packet.eid = htonl(eid);
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));

  return packet;
}

hxb_packet_int8 Packet::write8(uint32_t eid, uint8_t datatype, uint8_t value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_int8 packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = htonl(eid);
  packet.datatype = datatype;
  packet.value = value;
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));

  return packet;
}

hxb_packet_int32 Packet::write32(uint32_t eid, uint8_t datatype, uint32_t value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_int32 packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = htonl(eid);
  packet.datatype = datatype;
  packet.value = htonl(value);
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));

  return packet;
}

hxb_packet_float Packet::writef(uint32_t eid, uint8_t datatype, float value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_float packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = htonl(eid);
  packet.datatype = datatype;
  uint32_t value_nbo = htonl(*(uint32_t*)&value);
  packet.value = *(float*)&value_nbo;
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));

  return packet;
}

hxb_packet_datetime Packet::writedt(uint32_t eid, uint8_t datatype, datetime value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_datetime packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = htonl(eid);
  packet.datatype = datatype;
  value.year = htons(value.year);
  packet.value = value;
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));

  return packet;
}

hxb_packet_66bytes Packet::writebytes(uint32_t eid, uint8_t datatype, const char* value, size_t length, bool broadcast)
{
  if(length > HXB_BYTES_PACKET_MAX_BUFFER_LENGTH)
    throw std::exception(); // TODO
  CRC::Ptr crc(new CRC());
  struct hxb_packet_66bytes packet;
  memset((char*)&packet.value, 0, HXB_BYTES_PACKET_MAX_BUFFER_LENGTH);
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = htonl(eid);
  packet.datatype = datatype;
  memcpy((char*)&packet.value, value, length);
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet) - 2));

  return packet;
}

hxb_packet_128string Packet::writestr(uint32_t eid, uint8_t datatype, std::string value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_128string packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = htonl(eid);
  packet.datatype = datatype;
  strncpy((char*)&packet.value, value.c_str(), 128);
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet) - 2));

  return packet;
}

PacketHandling::PacketHandling(char* data)
{
  CRC::Ptr crc(new hexabus::CRC());

  okay = crc_okay = false;
  packet_type = datatype = eid = 0;
  value.datatype = 0;

  struct hxb_packet_header* header = (struct hxb_packet_header*)data;

  if(strncmp((char*)header, HXB_HEADER, 4))
  {
    okay = false;
  } else {
    okay = true;
    packet_type = header->type;
    if(header->type == HXB_PTYPE_INFO || header->type == HXB_PTYPE_WRITE)
    {
      datatype = header->datatype;
      value.datatype = datatype;
      switch(datatype)
      {
        case HXB_DTYPE_UNDEFINED:
          break;
        case HXB_DTYPE_BOOL:
        case HXB_DTYPE_UINT8:
          {
            struct hxb_packet_int8* packet8 = (struct hxb_packet_int8*)data;
            packet8->crc = ntohs(packet8->crc);
            crc_okay = packet8->crc == crc->crc16((char*)packet8, sizeof(*packet8)-2);

            eid = ntohl(packet8->eid);
            *(uint8_t*)&value.data = packet8->value;
          }
          break;
        case HXB_DTYPE_UINT32:
          {
            struct hxb_packet_int32* packet32 = 
              (struct hxb_packet_int32*)data;
            packet32->crc = ntohs(packet32->crc);
            crc_okay = packet32->crc == crc->crc16((char*)packet32, sizeof(*packet32)-2); // ntohl the value after the CRC check, CRC check is done with everything in network byte order
            packet32->value = ntohl(packet32->value);

            eid = ntohl(packet32->eid);
            memset(value.data, 0, sizeof(value.data));
            memcpy(&value.data[0], &packet32->value, sizeof(uint32_t));
          }
          break;
        case HXB_DTYPE_DATETIME:
          {
            struct hxb_packet_datetime* packetdt = (struct hxb_packet_datetime*)data;
            packetdt->crc = ntohs(packetdt->crc);
            crc_okay = packetdt->crc == crc->crc16((char*)packetdt, sizeof(*packetdt)-2);
            packetdt->value.year = ntohs(packetdt->value.year);

            eid = ntohl(packetdt->eid);
            *(datetime*)&value.data = packetdt->value;
          }
          break;
        case HXB_DTYPE_FLOAT:

          {
            struct hxb_packet_float* packetf = (struct hxb_packet_float*)data;
            packetf->crc = ntohs(packetf->crc);
            crc_okay = packetf->crc == crc->crc16((char*)packetf, sizeof(*packetf)-2);
            uint32_t value_hbo = ntohl(*(uint32_t*)&packetf->value);
            packetf->value = *(float*)&value_hbo;

            eid = ntohl(packetf->eid);
            memset(value.data, 0, sizeof(value.data));
            memcpy(&value.data[0], &packetf->value, sizeof(float));
          }
          break;
        case HXB_DTYPE_128STRING:
          {
            struct hxb_packet_128string* packetstr = (struct hxb_packet_128string*)data;
            packetstr->crc = ntohs(packetstr->crc);
            crc_okay = packetstr->crc == crc->crc16((char*)packetstr, sizeof(*packetstr)-2);

            // make sure it's not too long
            packetstr->value[127] = '\0';

            eid = ntohl(packetstr->eid);
            strval = packetstr->value;
          }
          break;
        default:
            // not implemented here
          break;
      }
    } else if(header->type == HXB_PTYPE_EPINFO) {
      struct hxb_packet_128string* packetepi = (struct hxb_packet_128string*)data;
      datatype = packetepi->datatype;
      eid = ntohl(packetepi->eid);
      crc_okay = ntohs(packetepi->crc) == crc->crc16((char*)packetepi, sizeof(*packetepi)-2);
      packetepi->value[127] = '\0'; // set last character of string to 0 in case someone forgot that
      strval = packetepi->value;
    } else if(header->type == HXB_PTYPE_ERROR) {
      struct hxb_packet_error* packet = (struct hxb_packet_error*)data;
      packet->crc = ntohs(packet->crc);
      crc_okay = packet->crc == crc->crc16((char*)packet, sizeof(*packet)-2);
      errorcode = packet->errorcode; 
		} else {
    	std::cout << "Packet Format not printable yet." << std::endl; // Packet type not implemented here
    }
  }
}

bool PacketHandling::getOkay()              { return okay; }
bool PacketHandling::getCRCOkay()           { return crc_okay; }
uint8_t PacketHandling::getPacketType()     { return packet_type; }
uint8_t PacketHandling::getErrorcode()      { return errorcode; }
uint8_t PacketHandling::getDatatype()       { return datatype; }
uint32_t PacketHandling::getEID()            { return eid; }
struct hxb_value PacketHandling::getValue() { return value; }
int PacketHandling::getValuePtr(struct hxb_value *v) {
   if (v)
   {
     memcpy(v, &value, sizeof(value));
     return 0;
   }
   else
   {
     return EINVAL;
   }
}
std::string PacketHandling::getString()     { return strval; }

