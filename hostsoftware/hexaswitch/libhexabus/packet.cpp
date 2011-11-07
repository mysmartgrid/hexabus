#include "packet.hpp"

#include "common.hpp"
#include "crc.hpp"

#include <string.h>
#include <netinet/in.h>

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

hxb_packet_int8 Packet::setvalue8(uint8_t eid, uint8_t datatype, uint8_t value, bool broadcast)
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

hxb_packet_int32 Packet::setvalue32(uint8_t eid, uint8_t datatype, uint32_t value, bool broadcast)
{
  CRC::Ptr crc(new CRC());
  struct hxb_packet_int32 packet;
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
