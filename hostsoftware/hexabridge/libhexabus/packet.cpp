#include "packet.hpp"

#include "common.hpp"
#include "crc.hpp"

#include <string.h>
#include <netinet/in.h>

#include <iostream>

using namespace hexabus;

hxb_packet_query Packet::query(uint8_t eid)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_query packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_QUERY;
  packet.flags = 0;
  packet.eid = eid;
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));
  // for test, output the Hexabus packet
  // printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.eid, ntohs(packet.crc));
  // TODO either use cout, or delete this.
  //TODO implement -v

  return packet;
}

hxb_packet_int8 Packet::write8(uint8_t eid, uint8_t datatype, uint8_t value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_int8 packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = eid;
  packet.datatype = datatype;
  packet.value = value;
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));
  // for test, output the Hexabus packet
  // TODO cout! printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.eid, packet.datatype, packet.value, packet.crc);

  return packet;
}

hxb_packet_int32 Packet::write32(uint8_t eid, uint8_t datatype, uint32_t value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_int32 packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = eid;
  packet.datatype = datatype;
  packet.value = htonl(value);
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));
  // for test, output the Hexabus packet
  // TODO cout! printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.eid, packet.datatype, packet.value, packet.crc);

  return packet;
}

hxb_packet_datetime Packet::writedt(uint8_t eid, uint8_t datatype, datetime value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_datetime packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = eid;
  packet.datatype = datatype;
  value.year = htons(value.year);
  packet.value = value;
  packet.crc = htons(crc->crc16((char*)&packet, sizeof(packet)-2));
  // for test, output the Hexabus packet
  // TODO cout! printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.eid, packet.datatype, packet.value, packet.crc);

  return packet;
}

PacketHandling::PacketHandling(char* data)
{
  CRC::Ptr crc(new hexabus::CRC());

  okay = crc_okay = false;
  packet_type = datatype = eid = 0;
  value.datatype = 0; 

  // TODO somehow make sure the data pointer points to something we can actually use
  struct hxb_packet_header* header = (struct hxb_packet_header*)data;

  if(strncmp((char*)header, HXB_HEADER, 4))
  {
    okay = false;
  } else {
    okay = true;
    packet_type = header->type;
    if(header->type == HXB_PTYPE_INFO || header->type == HXB_PTYPE_WRITE)
    {
      // Declare pointers here - but use only the relevant one in the switch()
      struct hxb_packet_int8* packet8;
      struct hxb_packet_int32* packet32;
      struct hxb_packet_datetime* packetdt;

      datatype = header->datatype;
      value.datatype = datatype;
      switch(datatype)
      {
        case HXB_DTYPE_UNDEFINED:
          break;
        case HXB_DTYPE_BOOL:
        case HXB_DTYPE_UINT8:
          packet8 = (struct hxb_packet_int8*)data;
          packet8->crc = ntohs(packet8->crc);
          crc_okay = packet8->crc == crc->crc16((char*)packet8, sizeof(*packet8)-2);

          eid = packet8->eid;
          value.int8 = packet8->value;
          break;
        case HXB_DTYPE_UINT32:
          packet32 = (struct hxb_packet_int32*)data;
          packet32->crc = ntohs(packet32->crc);
          crc_okay = packet32->crc == crc->crc16((char*)packet32, sizeof(*packet32)-2);
          // ntohl the value after the CRC check, CRC check is done with everything in network byte order
          packet32->value = ntohl(packet32->value);

          eid = packet32->eid;
          value.int32 = packet32->value;
          break;
        case HXB_DTYPE_DATETIME:
          packetdt = (struct hxb_packet_datetime*)data;
          packetdt->crc = ntohs(packetdt->crc);
          crc_okay = packetdt->crc == crc->crc16((char*)packetdt, sizeof(*packetdt)-2);
          packetdt->value.year = ntohs(packetdt->value.year);

          eid = packetdt->eid;
          value.datetime = packetdt->value;
        default:
          // datatype not implemented here
          break;
      }
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
uint8_t PacketHandling::getEID()            { return eid; }
struct hxb_value PacketHandling::getValue() { return value; }

