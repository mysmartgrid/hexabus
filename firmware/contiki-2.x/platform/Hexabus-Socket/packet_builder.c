#include "packet_builder.h"
#include <string.h>
#include "hexabus_config.h"
#include "net/uip.h"
#include "lib/crc16.h"
#include "endpoint_registry.h"

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

static void finalize_header(struct hxb_eidpacket_header* header)
{
	strncpy(header->magic, HXB_HEADER, strlen(HXB_HEADER));
	header->flags = 0;
	header->eid = uip_htonl(header->eid);
}

void packet_finalize_u8(struct hxb_packet_u8* packet)
{
	finalize_header((struct hxb_eidpacket_header*) packet);
	packet->crc = crc16_data((unsigned char*) packet, sizeof(*packet) - 2, 0);
}

void packet_finalize_u32(struct hxb_packet_u32* packet)
{
	finalize_header((struct hxb_eidpacket_header*) packet);

	packet->value = uip_htonl(packet->value);
	packet->crc = crc16_data((unsigned char*) packet, sizeof(*packet) - 2, 0);
}

void packet_finalize_datetime(struct hxb_packet_datetime* packet)
{
	finalize_header((struct hxb_eidpacket_header*) packet);

	packet->value.year = uip_htons(packet->value.year);
	packet->crc = crc16_data((unsigned char*) packet, sizeof(*packet) - 2, 0);
}

void packet_finalize_float(struct hxb_packet_float* packet)
{
	finalize_header((struct hxb_eidpacket_header*) packet);

	union {
		float f;
		uint32_t u;
	} fconv;

	fconv.f = packet->value;
	fconv.u = uip_htonl(fconv.u);

	packet->value = fconv.f;
	packet->crc = crc16_data((unsigned char*) packet, sizeof(*packet) - 2, 0);
}

void packet_finalize_128string(struct hxb_packet_128string* packet)
{
	finalize_header((struct hxb_eidpacket_header*) packet);
	packet->crc = crc16_data((unsigned char*) packet, sizeof(*packet) - 2, 0);
}

void packet_finalize_66bytes(struct hxb_packet_66bytes* packet)
{
	finalize_header((struct hxb_eidpacket_header*) packet);
	packet->crc = crc16_data((unsigned char*) packet, sizeof(*packet) - 2, 0);
}

void packet_finalize_16bytes(struct hxb_packet_16bytes* packet)
{
	finalize_header((struct hxb_eidpacket_header*) packet);
	packet->crc = crc16_data((unsigned char*) packet, sizeof(*packet) - 2, 0);
}

struct hxb_packet_error make_error_packet(enum hxb_error_code code)
{
	struct hxb_packet_error packet;
	strncpy(packet.magic, HXB_HEADER, strlen(HXB_HEADER));
	packet.type = HXB_PTYPE_ERROR;

	packet.flags = 0;
	packet.error_code = code;
	packet.crc = uip_htons(crc16_data((unsigned char*)&packet, sizeof(packet) - 2, 0));

	return packet;
}

