#ifndef HEXABUS_PACKET_H_
#define HEXABUS_PACKET_H_

// Hexabus packet definition
#include <stdint.h>
#include "hexabus_definitions.h"
#include "hexabus_types.h"

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

// used for BOOL und UINT(
struct hxb_packet_u8 {
	HXB_VALUEPACKET_HEADER
	uint8_t value;
	HXB_PACKET_FOOTER
} __attribute__((packed));

// used for UINT32 and TIMESTAMP
struct hxb_packet_u32 {
	HXB_VALUEPACKET_HEADER
	uint32_t value;
	HXB_PACKET_FOOTER
} __attribute__((packed));

struct hxb_packet_datetime {
	HXB_VALUEPACKET_HEADER
	struct hxb_datetime value;
	HXB_PACKET_FOOTER
} __attribute__((packed));

struct hxb_packet_float {
	HXB_VALUEPACKET_HEADER
	float value;
	HXB_PACKET_FOOTER
} __attribute__((packed));

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
	struct hxb_packet_u32 p_u32;
	struct hxb_packet_datetime p_datetime;
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
