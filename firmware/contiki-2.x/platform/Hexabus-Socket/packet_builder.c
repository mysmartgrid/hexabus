#include "packet_builder.h"
#include <string.h>
#include "hexabus_config.h"
#include "net/uip.h"

#if PACKET_BUILDER_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif



//
//static void  //TODO remove once everything is using the new format
//make_message(char* buf, uint16_t command, uint16_t value)
//{
//  struct hexabusmsg_t *hexabuscmd;
//  hexabuscmd = (struct hexabusmsg_t *)buf;
//  memset(hexabuscmd, 0, sizeof(struct hexabusmsg_t));
//  strncpy(hexabuscmd->header, HEXABUS_HEADER, sizeof(hexabuscmd->header));
//  hexabuscmd->source = 2;
//  hexabuscmd->command = uip_htons(command);
//  uint16_t *response;
//  response = (uint16_t*) (hexabuscmd + 1);
//  *response = uip_htons(value);
//  PRINTF("udp_handler: Responding with message: ");
//  PRINTF("%s%02x%02x%04d\n", HEXABUS_HEADER, 2, command, value);
//}
//
struct hxb_packet_float make_value_packet_float(uint32_t eid, struct hxb_value* val)
{
  struct hxb_packet_float packet;
  strncpy(&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_INFO;
  packet.flags = 0;
  PRINTF("PACKET BUILDER EID: %d\n", eid);
  packet.eid = uip_htonl(eid);

  packet.datatype = val->datatype;
  // uip_htonl works on 32bit-int
  uint32_t value_nbo = uip_htonl(*(uint32_t*)&val->data);
  packet.value = *(float*)&value_nbo;

  packet.crc = uip_htons(crc16_data((char*)&packet, sizeof(packet)-2, 0));
  PRINTF("Build packet:\n\nType:\t%d\r\nFlags:\t%d\r\nEID:\t%d\r\nValue:\t%lx\r\nCRC:\t%u\r\n\r\n",
    packet.type, packet.flags, uip_ntohl(packet.eid), packet.value, uip_ntohs(packet.crc)); // printf can handle float?
  return packet;
}

struct hxb_packet_int32 make_value_packet_int32(uint32_t eid, struct hxb_value* val)
{
  struct hxb_packet_int32 packet;
  strncpy(&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_INFO;
  packet.flags = 0;
  packet.eid = uip_htonl(eid);

  packet.datatype = val->datatype;
  packet.value = uip_htonl(*(uint32_t*)&val->data);

  packet.crc = uip_htons(crc16_data((char*)&packet, sizeof(packet)-2, 0));
  PRINTF("Build packet:\n\nType:\t%d\r\nFlags:\t%d\r\nEID:\t%ld\r\nValue:\t%d\r\nCRC:\t%u\r\n\r\n",
    packet.type, packet.flags, uip_ntohl(packet.eid), uip_ntohl(packet.value), uip_ntohs(packet.crc)); // printf behaves strange here?
  return packet;
}

struct hxb_packet_128string make_epinfo_packet(uint32_t eid)
{
  struct hxb_packet_128string packet;
  strncpy(&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_EPINFO;
  packet.flags = 0;
  packet.eid = uip_htonl(eid);

  packet.datatype = endpoint_get_datatype(packet.eid);
  endpoint_get_name(packet.eid, &packet.value);

  packet.crc = uip_htons(crc16_data((char*)&packet, sizeof(packet)-2, 0));


  return packet;
}

struct hxb_packet_int8 make_value_packet_int8(uint32_t eid, struct hxb_value* val)
{
  struct hxb_packet_int8 packet;
  strncpy(&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_INFO;
  packet.flags = 0;
  packet.eid = uip_htonl(eid);

  packet.datatype = val->datatype;
  packet.value = val->data[0];

  packet.crc = uip_htons(crc16_data((char*)&packet, sizeof(packet)-2, 0));

  PRINTF("Build packet:\nType:\t%d\r\nFlags:\t%d\r\nEID:\t%u\r\n",
          packet.type, packet.flags, uip_ntohl(packet.eid));
  PRINTF("Dtype:\t%d\r\nValue:\t%d\r\nCRC:\t%u\r\n",
          packet.datatype, packet.value, uip_ntohs(packet.crc));

  return packet;
}

struct hxb_packet_error make_error_packet(uint8_t errorcode)
{
  struct hxb_packet_error packet;
  strncpy(&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_ERROR;

  packet.flags = 0;
  packet.errorcode = errorcode;
  packet.crc = uip_htons(crc16_data((char*)&packet, sizeof(packet)-2, 0));

  PRINTF("Built packet:\r\nType:\t%d\r\nFlags:\t%d\r\nError Code:\t%d\r\nCRC:\t%u\r\n\r\n",
    packet.type, packet.flags, packet.errorcode, uip_ntohs(packet.crc));

  return packet;
}
