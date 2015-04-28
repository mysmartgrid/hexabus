#ifndef HEXABUS_PACKET_H_
#define HEXABUS_PACKET_H_

// Hexabus packet definition
#include <stdint.h>
#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/hexabus_types.h"

#include "net/uip.h"

#define HXB_PACKET_HEADER \
	char     magic[4];       /* HXB_HEADER, of course      */\
	uint8_t  type;           /* actually enum hxb_datatype */\
	uint8_t  flags;          /* e.g. for requesting ACKs   */\
	uint16_t sequence_number; /* for reliable transmission  */

#define HXB_CAUSE_FOOTER \
	uint16_t cause_sequence_number; \

struct hxb_packet_header {
	HXB_PACKET_HEADER
} __attribute__((packed));

#define HXB_EIDPACKET_HEADER \
	HXB_PACKET_HEADER \
	uint32_t eid;

struct hxb_eidpacket_header {
	HXB_EIDPACKET_HEADER
} __attribute__((packed));

struct hxb_packet_ack {
	HXB_PACKET_HEADER
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_packet_error {
	HXB_PACKET_HEADER
	uint8_t error_code;            /* actually enum hxb_error_code */
	HXB_CAUSE_FOOTER
} __attribute__((packed));

// used for QUERY and EPQUERY
struct hxb_packet_query {
	HXB_EIDPACKET_HEADER
} __attribute__((packed));

struct hxb_packet_property_query {
	HXB_EIDPACKET_HEADER
	uint32_t property_id;
} __attribute__((packed));

/* INFO, EPINFO */

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

} __attribute__((packed));

struct hxb_packet_65bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_65BYTES_PACKET_BUFFER_LENGTH];

} __attribute__((packed));

struct hxb_packet_16bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_16BYTES_PACKET_BUFFER_LENGTH];

} __attribute__((packed));

/* PINFO */

#define HXB_PROXYPACKET(suffix, type) \
	struct hxb_proxy_packet_##suffix { \
		HXB_VALUEPACKET_HEADER \
		type value; \
		char origin[16]; \
	} __attribute__((packed));

HXB_PROXYPACKET(u8, uint8_t)
HXB_PROXYPACKET(u16, uint16_t)
HXB_PROXYPACKET(u32, uint32_t)
HXB_PROXYPACKET(u64, uint64_t)
HXB_PROXYPACKET(s8, int8_t)
HXB_PROXYPACKET(s16, int16_t)
HXB_PROXYPACKET(s32, int32_t)
HXB_PROXYPACKET(s64, int64_t)
HXB_PROXYPACKET(float, float)

#undef HXB_PROXYPACKET

struct hxb_proxy_packet_128string {
	HXB_VALUEPACKET_HEADER
	char value[HXB_STRING_PACKET_BUFFER_LENGTH + 1];
	char origin[16];

} __attribute__((packed));

struct hxb_proxy_packet_65bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_65BYTES_PACKET_BUFFER_LENGTH];
	char origin[16];

} __attribute__((packed));

struct hxb_proxy_packet_16bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_16BYTES_PACKET_BUFFER_LENGTH];
	char origin[16];

} __attribute__((packed));

/* PROPERTY WRITE */

#define HXB_PROPERTY_HEADER \
	HXB_VALUEPACKET_HEADER \
	uint32_t property_id;

struct hxb_property_header {
	HXB_PROPERTY_HEADER
} __attribute__((packed));

#define HXB_PROPWRITEPACKET(suffix, type) \
	struct hxb_property_write_##suffix { \
		HXB_PROPERTY_HEADER \
		type value; \
	} __attribute__((packed));

HXB_PROPWRITEPACKET(u8, uint8_t)
HXB_PROPWRITEPACKET(u16, uint16_t)
HXB_PROPWRITEPACKET(u32, uint32_t)
HXB_PROPWRITEPACKET(u64, uint64_t)
HXB_PROPWRITEPACKET(s8, int8_t)
HXB_PROPWRITEPACKET(s16, int16_t)
HXB_PROPWRITEPACKET(s32, int32_t)
HXB_PROPWRITEPACKET(s64, int64_t)
HXB_PROPWRITEPACKET(float, float)

#undef HXB_PROPWRITEPACKET

struct hxb_property_write_128string {
	HXB_PROPERTY_HEADER
	char value[HXB_STRING_PACKET_BUFFER_LENGTH + 1];

} __attribute__((packed));

struct hxb_property_write_65bytes {
	HXB_PROPERTY_HEADER
	char value[HXB_65BYTES_PACKET_BUFFER_LENGTH];

} __attribute__((packed));

struct hxb_property_write_16bytes {
	HXB_PROPERTY_HEADER
	char value[HXB_16BYTES_PACKET_BUFFER_LENGTH];

} __attribute__((packed));

/* PROP_REPORT */

#define HXB_PROPREPORTPACKET_HEADER \
	HXB_VALUEPACKET_HEADER \
	uint32_t next_id;

struct hxb_propreport_header {
	HXB_PROPREPORTPACKET_HEADER
} __attribute__((packed));

#define HXB_PROPREPORTPACKET(suffix, type) \
	struct hxb_property_report_##suffix { \
		HXB_PROPREPORTPACKET_HEADER \
		type value; \
		HXB_CAUSE_FOOTER \
	} __attribute__((packed));

HXB_PROPREPORTPACKET(u8, uint8_t)
HXB_PROPREPORTPACKET(u16, uint16_t)
HXB_PROPREPORTPACKET(u32, uint32_t)
HXB_PROPREPORTPACKET(u64, uint64_t)
HXB_PROPREPORTPACKET(s8, int8_t)
HXB_PROPREPORTPACKET(s16, int16_t)
HXB_PROPREPORTPACKET(s32, int32_t)
HXB_PROPREPORTPACKET(s64, int64_t)
HXB_PROPREPORTPACKET(float, float)

#undef HXB_PROPREPORTPACKET

struct hxb_property_report_128string {
	HXB_PROPREPORTPACKET_HEADER
	char value[HXB_STRING_PACKET_BUFFER_LENGTH + 1];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_property_report_65bytes {
	HXB_PROPREPORTPACKET_HEADER
	char value[HXB_65BYTES_PACKET_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_property_report_16bytes {
	HXB_PROPREPORTPACKET_HEADER
	char value[HXB_16BYTES_PACKET_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

/* REPORT, EPREPORT */

#define HXB_REPORTPACKET(suffix, type) \
	struct hxb_report_##suffix { \
		HXB_VALUEPACKET_HEADER \
		type value; \
		HXB_CAUSE_FOOTER \
	} __attribute__((packed));

HXB_REPORTPACKET(u8, uint8_t)
HXB_REPORTPACKET(u16, uint16_t)
HXB_REPORTPACKET(u32, uint32_t)
HXB_REPORTPACKET(u64, uint64_t)
HXB_REPORTPACKET(s8, int8_t)
HXB_REPORTPACKET(s16, int16_t)
HXB_REPORTPACKET(s32, int32_t)
HXB_REPORTPACKET(s64, int64_t)
HXB_REPORTPACKET(float, float)

struct hxb_report_128string {
	HXB_VALUEPACKET_HEADER
	char value[HXB_STRING_PACKET_BUFFER_LENGTH + 1];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_report_65bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_65BYTES_PACKET_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_report_16bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_16BYTES_PACKET_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));


union hxb_packet_any {
	struct hxb_packet_header header;
	struct hxb_eidpacket_header eid_header;
	struct hxb_valuepacket_header value_header;
	struct hxb_property_header property_header;
	struct hxb_propreport_header propreport_header;

	struct hxb_packet_error p_error;
	struct hxb_packet_query p_query;
	struct hxb_packet_property_query p_pquery;
	struct hxb_packet_ack p_ack;

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

	struct hxb_proxy_packet_u8 p_proxy_u8;
	struct hxb_proxy_packet_u16 p_proxy_u16;
	struct hxb_proxy_packet_u32 p_proxy_u32;
	struct hxb_proxy_packet_u64 p_proxy_u64;
	struct hxb_proxy_packet_s8 p_proxy_s8;
	struct hxb_proxy_packet_s16 p_proxy_s16;
	struct hxb_proxy_packet_s32 p_proxy_s32;
	struct hxb_proxy_packet_s64 p_proxy_s64;
	struct hxb_proxy_packet_float p_proxy_float;
	struct hxb_proxy_packet_128string p_proxy_128string;
	struct hxb_proxy_packet_65bytes p_proxy_65bytes;
	struct hxb_proxy_packet_16bytes p_proxy_16bytes;

	struct hxb_property_write_u8 p_pwrite_u8;
	struct hxb_property_write_u16 p_pwrite_u16;
	struct hxb_property_write_u32 p_pwrite_u32;
	struct hxb_property_write_u64 p_pwrite_u64;
	struct hxb_property_write_s8 p_pwrite_s8;
	struct hxb_property_write_s16 p_pwrite_s16;
	struct hxb_property_write_s32 p_pwrite_s32;
	struct hxb_property_write_s64 p_pwrite_s64;
	struct hxb_property_write_float p_pwrite_float;
	struct hxb_property_write_128string p_pwrite_128string;
	struct hxb_property_write_65bytes p_pwrite_65bytes;
	struct hxb_property_write_16bytes p_pwrite_16bytes;

	struct hxb_property_report_u8 p_preport_u8;
	struct hxb_property_report_u16 p_preport_u16;
	struct hxb_property_report_u32 p_preport_u32;
	struct hxb_property_report_u64 p_preport_u64;
	struct hxb_property_report_s8 p_preport_s8;
	struct hxb_property_report_s16 p_preport_s16;
	struct hxb_property_report_s32 p_preport_s32;
	struct hxb_property_report_s64 p_preport_s64;
	struct hxb_property_report_float p_preport_float;
	struct hxb_property_report_128string p_preport_128string;
	struct hxb_property_report_65bytes p_preport_65bytes;
	struct hxb_property_report_16bytes p_preport_16bytes;

	struct hxb_report_u8 p_report_u8;
	struct hxb_report_u16 p_report_u16;
	struct hxb_report_u32 p_report_u32;
	struct hxb_report_u64 p_report_u64;
	struct hxb_report_s8 p_report_s8;
	struct hxb_report_s16 p_report_s16;
	struct hxb_report_s32 p_report_s32;
	struct hxb_report_s64 p_report_s64;
	struct hxb_report_float p_report_float;
	struct hxb_report_128string p_report_128string;
	struct hxb_report_65bytes p_report_65bytes;
	struct hxb_report_16bytes p_report_16bytes;
};



// ======================================================================
// Structs for passing Hexabus data around between processes
// Since there the IP information is lost, we need a field for the IP address of the sender/receiver.

struct hxb_envelope {
	uip_ipaddr_t      src_ip;
	uint16_t          src_port;
	uint32_t          eid;
	struct hxb_value  value;
};

struct hxb_queue_packet {
	uip_ipaddr_t			ip;
	uint16_t				port;
	union hxb_packet_any	packet;
};

#undef HXB_PACKET_HEADER
#undef HXB_EIDPACKET_HEADER
#undef HXB_CAUSE_FOOTER
#undef HXB_VALUEPACKET_HEADER
#undef HXB_PROPERTY_HEADER
#undef HXB_PROPREPORTPACKET_HEADER

#endif
