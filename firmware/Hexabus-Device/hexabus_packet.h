#ifndef HEXABUS_PACKET_H_
#define HEXABUS_PACKET_H_

// Hexabus packet definition
#include <stdint.h>
#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/hexabus_types.h"

#include "net/uip.h"

#define HXB_PACKET_HEADER \
	char    magic[4];   /* HXB_HEADER, of course      */\
	uint8_t type;       /* actually enum hxb_datatype */\
	uint8_t flags;      /* unused                     */

#define HXB_PACKET_FOOTER \
	uint16_t crc;                        /* CRC16-Kermit / Contiki's crc16_data() */

struct hxb_packet_header {
	HXB_PACKET_HEADER
} __attribute__((packed));

#define HXB_EIDPACKET_HEADER \
	HXB_PACKET_HEADER \
	uint32_t eid;

struct hxb_eidpacket_header {
	HXB_EIDPACKET_HEADER
} __attribute__((packed));

struct hxb_packet_error {
	HXB_PACKET_HEADER
	uint8_t error_code;                  /* actually enum hxb_error_code */
	HXB_PACKET_FOOTER
} __attribute__((packed));

// used for QUERY and EPQUERY
struct hxb_packet_query {
	HXB_EIDPACKET_HEADER
	HXB_PACKET_FOOTER
} __attribute__((packed));

#define HXB_VALUEPACKET_HEADER \
	HXB_EIDPACKET_HEADER \
	uint8_t datatype;                    /* actually enum hxb_datatype */

struct hxb_valuepacket_header {
	HXB_VALUEPACKET_HEADER
} __attribute__((packed));

#define HXB_VALPACKET(suffix, type) \
	struct hxb_packet_##suffix { \
		HXB_VALUEPACKET_HEADER \
		type value; \
		HXB_PACKET_FOOTER \
	} __attribute__((packed));

HXB_VALPACKET(u8, uint8_t)
HXB_VALPACKET(u16, uint16_t)
HXB_VALPACKET(u32, uint32_t)
HXB_VALPACKET(u64, uint64_t)
HXB_VALPACKET(s8, int8_t)
HXB_VALPACKET(s16, int16_t)
HXB_VALPACKET(s32, int32_t)
HXB_VALPACKET(s64, int64_t)
HXB_VALPACKET(float, float)

#undef HXB_VALPACKET

struct hxb_packet_128string {
	HXB_VALUEPACKET_HEADER
	char value[HXB_STRING_PACKET_BUFFER_LENGTH + 1];
	HXB_PACKET_FOOTER
} __attribute__((packed));

struct hxb_packet_65bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_65BYTES_PACKET_BUFFER_LENGTH];
	HXB_PACKET_FOOTER
} __attribute__((packed));

struct hxb_packet_16bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_16BYTES_PACKET_BUFFER_LENGTH];
	HXB_PACKET_FOOTER
} __attribute__((packed));



union hxb_packet_any {
	struct hxb_packet_header header;
	struct hxb_eidpacket_header eid_header;
	struct hxb_valuepacket_header value_header;

	struct hxb_packet_error p_error;
	struct hxb_packet_query p_query;
	struct hxb_packet_u8 p_u8;
	struct hxb_packet_u16 p_u16;
	struct hxb_packet_u32 p_u32;
	struct hxb_packet_u64 p_u64;
	struct hxb_packet_s8 p_s8;
	struct hxb_packet_s16 p_s16;
	struct hxb_packet_s32 p_s32;
	struct hxb_packet_s64 p_s64;
	struct hxb_packet_float p_float;
	struct hxb_packet_128string p_128string;
	struct hxb_packet_65bytes p_65bytes;
	struct hxb_packet_16bytes p_16bytes;
};



// ======================================================================
// Structs for passing Hexabus data around between processes
// Since there the IP information is lost, we need a field for the IP address of the sender/receiver. But we can drop the CRC here.

struct hxb_envelope {
	uip_ipaddr_t      src_ip;
	uint16_t          src_port;
	uint32_t          eid;
	struct hxb_value  value;
};

#undef HXB_PACKET_HEADER
#undef HXB_PACKET_FOOTER
#undef HXB_EIDPACKET_HEADER
#undef HXB_VALUEPACKET_HEADER

#endif
