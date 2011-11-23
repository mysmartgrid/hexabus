#ifndef HEXABUS_PACKET_H_
#define HEXABUS_PACKET_H_

// Hexabus packet definition
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

// ======================================================================
// Struct for passing Hexabus values around
// One struct for all data types, with a datatype flag indicating which
// of the values is used. Used for passing values to and from
// endpoint_access
struct hxb_value {
  uint8_t   datatype;   // Datatype that is used, or HXB_DTYPE_UNDEFINED
  uint8_t   int8;       // used for HXB_DTYPE_BOOL and HXB_DTYPE_UINT8
  uint32_t  int32;      // used for HXB_DTYPE_UINT32
  float     float32;      // used for HXB_DTYPE_FLOAT
  struct datetime  datetime;   // used for HXB_DTYPE_DATETIME
};

// ======================================================================
// Structs for passing Hexabus data around between processes
// Since there the IP information is lost, we need a field for the IP address of the sender/receiver. But we can drop the CRC here.

struct hxb_data {
    char      source[16];
    uint8_t   eid;
    struct hxb_value value;
};

// ======================================================================
// Definitions for fields

#define HXB_PORT              61616
#define HXB_HEADER            "HX0B"

// Boolean values
#define HXB_TRUE              1
#define HXB_FALSE             0
// TODO Do we need a "toggle"?

// Packet types
#define HXB_PTYPE_ERROR       0x00  // An error occured -- check the error code field for more information
#define HXB_PTYPE_INFO        0x01  // Endpoint provides information
#define HXB_PTYPE_QUERY       0x02  // Endpoint is requested to provide information
#define HXB_PTYPE_WRITE       0x04  // Endpoint is requested to set its value

// Flags
#define HXB_FLAG_CONFIRM      0x01  // Requests an acknowledgement

// Data types
#define HXB_DTYPE_UNDEFINED   0x00  // Undefined: Nonexistent endpoint
#define HXB_DTYPE_BOOL        0x01  // Boolean. Value still represented by 8 bits, but may only be HXB_TRUE or HXB_FALSE
#define HXB_DTYPE_UINT8       0x02  // Unsigned 8 bit integer
#define HXB_DTYPE_UINT32      0x03  // Unsigned 32 bit integer
#define HXB_DTYPE_DATETIME    0x04  // Date and time
#define HXB_DTYPE_FLOAT       0x05  // 32bit floating point

// Error codes
//                            0x00     reserved: No error
#define HXB_ERR_UNKNOWNEID    0x01  // A request for an endpoint which does not exist on the device was received
#define HXB_ERR_WRITEREADONLY 0x02  // A WRITE was received for a readonly endpoint
#define HXB_ERR_CRCFAILED     0x03  // A packet failed the CRC check -- TODO How can we find out what information was lost?
#define HXB_ERR_DATATYPE      0x04  // A packet with a datatype that does not fit the endpoint was received

#endif
