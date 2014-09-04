#ifndef HEXABUS_PACKET_H_
#define HEXABUS_PACKET_H_

// Hexabus packet definition
#include <stdint.h>
#include "hexabus_definitions.h"
#include "hexabus_types.h"

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
	uint8_t  error_code;            /* actually enum hxb_error_code */
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_packet_timeinfo {
	HXB_PACKET_HEADER
	struct hxb_datetime value;

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

// used for BOOL und UINT(
struct hxb_packet_u8 {
	HXB_VALUEPACKET_HEADER
	uint8_t value;

} __attribute__((packed));

// used for UINT32 and TIMESTAMP
struct hxb_packet_u32 {
	HXB_VALUEPACKET_HEADER
	uint32_t value;

} __attribute__((packed));

struct hxb_packet_datetime {
	HXB_VALUEPACKET_HEADER
	struct hxb_datetime value;

} __attribute__((packed));

struct hxb_packet_float {
	HXB_VALUEPACKET_HEADER
	float value;

} __attribute__((packed));

struct hxb_packet_128string {
	HXB_VALUEPACKET_HEADER
	char value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH + 1];

} __attribute__((packed));

struct hxb_packet_66bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH];

} __attribute__((packed));

struct hxb_packet_16bytes {
	HXB_VALUEPACKET_HEADER
	char value[HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH];

} __attribute__((packed));

/* PROPERTY WRITE */

#define HXB_PROPERTY_HEADER \
	HXB_VALUEPACKET_HEADER \
	uint32_t property_id;

struct hxb_property_header {
	HXB_PROPERTY_HEADER
} __attribute__((packed));

// used for BOOL und UINT(
struct hxb_property_write_u8 {
	HXB_PROPERTY_HEADER
	uint8_t value;

} __attribute__((packed));

// used for UINT32 and TIMESTAMP
struct hxb_property_write_u32 {
	HXB_PROPERTY_HEADER
	uint32_t value;

} __attribute__((packed));

struct hxb_property_write_datetime {
	HXB_PROPERTY_HEADER
	struct hxb_datetime value;

} __attribute__((packed));

struct hxb_property_write_float {
	HXB_PROPERTY_HEADER
	float value;

} __attribute__((packed));

struct hxb_property_write_128string {
	HXB_PROPERTY_HEADER
	char value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH + 1];

} __attribute__((packed));

struct hxb_property_write_66bytes {
	HXB_PROPERTY_HEADER
	char value[HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH];

} __attribute__((packed));

struct hxb_property_write_16bytes {
	HXB_PROPERTY_HEADER
	char value[HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH];

} __attribute__((packed));

/* PROP_REPORT */

#define HXB_PROPREPORTPACKET_HEADER \
	HXB_VALUEPACKET_HEADER \
	uint32_t next_id;

struct hxb_propreport_header {
	HXB_PROPREPORTPACKET_HEADER
} __attribute__((packed));

// used for BOOL und UINT8
struct hxb_property_report_u8 {
	HXB_PROPREPORTPACKET_HEADER
	uint8_t value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

// used for UINT32 and TIMESTAMP
struct hxb_property_report_u32 {
	HXB_PROPREPORTPACKET_HEADER
	uint32_t value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_property_report_datetime {
	HXB_PROPREPORTPACKET_HEADER
	struct hxb_datetime value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_property_report_float {
	HXB_PROPREPORTPACKET_HEADER
	float value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_property_report_128string {
	HXB_PROPREPORTPACKET_HEADER
	char value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH + 1];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_property_report_66bytes {
	HXB_PROPREPORTPACKET_HEADER
	char value[HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_property_report_16bytes {
	HXB_PROPREPORTPACKET_HEADER
	char value[HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

/* REPORT, EPREPORT */

// used for BOOL und UINT8
struct hxb_packet_u8_re {
	HXB_VALUEPACKET_HEADER
	uint8_t value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

// used for UINT32 and TIMESTAMP
struct hxb_packet_u32_re {
	HXB_VALUEPACKET_HEADER
	uint32_t value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_packet_datetime_re {
	HXB_VALUEPACKET_HEADER
	struct hxb_datetime value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_packet_float_re {
	HXB_VALUEPACKET_HEADER
	float value;
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_packet_128string_re {
	HXB_VALUEPACKET_HEADER
	char value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH + 1];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_packet_66bytes_re {
	HXB_VALUEPACKET_HEADER
	char value[HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));

struct hxb_packet_16bytes_re {
	HXB_VALUEPACKET_HEADER
	char value[HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH];
	HXB_CAUSE_FOOTER
} __attribute__((packed));


union hxb_packet_any {
	struct hxb_packet_header header;
	struct hxb_eidpacket_header eid_header;
	struct hxb_valuepacket_header value_header;
	struct hxb_property_header property_header;
	struct hxb_propreport_header propreport_header;

	struct hxb_packet_timeinfo p_timeinfo;
	struct hxb_packet_error p_error;
	struct hxb_packet_query p_query;
	struct hxb_packet_property_query p_pquery;
	struct hxb_packet_ack p_ack;
	struct hxb_packet_u8 p_u8;
	struct hxb_packet_u32 p_u32;
	struct hxb_packet_datetime p_datetime;
	struct hxb_packet_float p_float;
	struct hxb_packet_128string p_128string;
	struct hxb_packet_66bytes p_66bytes;
	struct hxb_packet_16bytes p_16bytes;

	struct hxb_property_write_u8 p_u8_prop;
	struct hxb_property_write_u32 p_u32_prop;
	struct hxb_property_write_datetime p_datetime_prop;
	struct hxb_property_write_float p_float_prop;
	struct hxb_property_write_128string p_128string_prop;
	struct hxb_property_write_66bytes p_66bytes_prop;
	struct hxb_property_write_16bytes p_16bytes_prop;

	struct hxb_property_report_u8 p_u8_prop_re;
	struct hxb_property_report_u32 p_u32_prop_re;
	struct hxb_property_report_datetime p_datetime_prop_re;
	struct hxb_property_report_float p_float_prop_re;
	struct hxb_property_report_128string p_128string_prop_re;
	struct hxb_property_report_66bytes p_66bytes_prop_re;
	struct hxb_property_report_16bytes p_16bytes_prop_re;

	struct hxb_packet_u8_re p_u8_re;
	struct hxb_packet_u32_re p_u32_re;
	struct hxb_packet_datetime_re p_datetime_re;
	struct hxb_packet_float_re p_float_re;
	struct hxb_packet_128string_re p_128string_re;
	struct hxb_packet_66bytes_re p_66bytes_re;
	struct hxb_packet_16bytes_re p_16bytes_re;
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

#endif
