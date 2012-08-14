#ifndef HEXABUS_PACKET_H_
#define HEXABUS_PACKET_H_

// Hexabus packet definition
#include "hexabus_definitions.h"
#include "hexabus_value.h"
#include <stdint.h>

// ======================================================================
// Supporting structs

// Struct containing time and date

struct datetime {
    uint8_t   hour;
    uint8_t   minute;
    uint8_t   second;
    uint8_t   day;
    uint8_t   month;
    uint16_t  year;
    uint8_t   weekday;  // numbers from 0 to 6, sunday as the first day of the week.
} __attribute__ ((packed));

// ======================================================================
// Structs for building and reading Hexabus packets

// Just the packet header. You can cast to a pointer to this in order to find out the packet type and flags
struct hxb_packet_header {
  char      header[4];  // HX0B
  uint8_t   type;       // Packet type
  uint8_t   flags;      // Flags
  uint8_t   eid;        // Endpoint ID / Error code if it's an error packet
  uint8_t   datatype;   // Datatype / first 8 bits of the CRC if it's an error packet
} __attribute__ ((packed));

// ERROR packet
struct hxb_packet_error {
  char      header[4];
  uint8_t   type;
  uint8_t   flags;
  uint8_t   errorcode;
  uint16_t  crc;
} __attribute__ ((packed));

// QUERY packet
struct hxb_packet_query {
  char      header[4];
  uint8_t   type;
  uint8_t   flags;
  uint8_t   eid;        // Endpoint ID
  uint16_t  crc;       // CRC16-Kermit / Contiki's crc16_data()
} __attribute__ ((packed));
// TODO this used to be hxb_packet_req

// Structs for WRITE and INFO packets -- since the payload comes in different sizes, we need several structs
// WRITE/INFO packet for BOOL and INT8
struct hxb_packet_int8 {
  char      header[4];
  uint8_t   type;
  uint8_t   flags;
  uint8_t   eid;
  uint8_t   datatype;
  uint8_t   value;
  uint16_t  crc;
} __attribute__ ((packed));
// TODO this used to be hxb_packet_bool and hxb_packet_int

// WRITE/INFO packet for INT32
struct hxb_packet_int32 {
  char      header[4];
  uint8_t   type;
  uint8_t   flags;
  uint8_t   eid;
  uint8_t   datatype;
  uint32_t  value;
  uint16_t  crc;
} __attribute__ ((packed));

// DATE/TIME
struct hxb_packet_datetime {
    char      header[4];
    uint8_t   type;
    uint8_t   flags;
    uint8_t   eid;
    uint8_t   datatype;
    struct datetime  value;
    uint16_t  crc;
} __attribute__ ((packed));

// WRITE/INFO packet for FLOAT -- basically the same format as int32, but with datatype float instead
// TODO can't we just use the packet_int32 here? We're casting it to int32 for byte order conversions anyway
struct hxb_packet_float {
  char      header[4];
  uint8_t   type;
  uint8_t   flags;
  uint8_t   eid;
  uint8_t   datatype;
  float     value;
  uint16_t  crc;
} __attribute__ ((packed));

// WRITE/INFO packet for 128 char fixed length string or EPINFO packet
struct hxb_packet_128string {
  char      header[4];
  uint8_t   type;
  uint8_t   flags;
  uint8_t   eid;
  uint8_t   datatype;     // this is set to the datatype of the endpoint if it's an EPINFO packet!
  char      value[128];
  uint16_t  crc;
} __attribute__ ((packed));
#define HXB_STRING_PACKET_MAX_BUFFER_LENGTH 127

// ======================================================================
// Structs for passing Hexabus data around between processes
// Since there the IP information is lost, we need a field for the IP address of the sender/receiver. But we can drop the CRC here.

struct hxb_envelope {
    char      source[16];
    uint8_t   eid;
    struct hxb_value value;
};

#endif
