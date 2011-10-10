#ifndef HEXABUS_PACKET_H_
#define HEXABUS_PACKET_H_

// Hexabus packet definition
#include <stdint.h>

#define HEXABUS_PORT 61616

struct hxb_packet_header { // just the packet header. You can cast to a pointer to this in order to find out the packet type and flags
  char    header[4]; // HX0B
  uint8_t type;      // packet type -- see below
  uint8_t flags;     // flags -- see below
  uint8_t vid;       // value ID, errorcode if it's an error packet.
} __attribute__ ((packed));

struct hxb_packet_req { // request a value
  char    header[4];
  uint8_t type;
  uint8_t flags;
  uint8_t vid;    // value ID
  uint16_t crc;   // CRC16-Kermit / Contiki's crc16_data()
} __attribute__ ((packed));

struct hxb_packet_bool { // set or broadcast a boolean value -- value can only be HXB_TRUE of HXB_FALSE
  char    header[4];
  uint8_t type;
  uint8_t flags;
  uint8_t vid;
  uint8_t datatype;
  uint8_t value;
  uint16_t crc;
} __attribute__ ((packed));

struct hxb_packet_int { // set or broadcast an integer value
  char    header[4];
  uint8_t type;
  uint8_t flags;
  uint8_t vid;
  uint8_t datatype;
  uint8_t value;
  uint16_t crc;
} __attribute__ ((packed));

struct hxb_packet_error {
  char    header[4];
  uint8_t type;
  uint8_t flags;
  uint8_t errorcode;
  uint16_t crc;
} __attribute__ ((packed));

// Data structure(s) for representig received values (broadcast or reply) on a device (for inter-process communication, where the IP information around the packet is gone)
struct hxb_data_int {
  char    source[16]; // IP address of the device that sent the broadcast
  uint8_t datatype;
  uint8_t vid;
  uint8_t value;
} __attribute__ ((packed));

// Packet header - Hexabus Packets must begin with this character sequence
#define HXB_HEADER "HX0B"

// Defines for bool-packet
#define HXB_TRUE  1
#define HXB_FALSE 0

// Defines for Packet types
#define HXB_PTYPE_ERROR     0x00  // An error occured. check the error code for more information
#define HXB_PTYPE_INFO      0x01  // Endpoint provides information -- VID refers to the VID on the sender
#define HXB_PTYPE_QUERY     0x02  // Endpoint is requested to provide information -- VID refers to the VID on the receiver
#define HXB_PTYPE_WRITE     0x03  // Endpoint is requested to set a value -- VID refers to the VID on the receiver

// Defines for flags
#define HXB_FLAG_CONFIRM    0x01  // requests an acknowledgement

// Defines for datatypes
#define HXB_DTYPE_BOOL      0x01  // Boolean -- value is represented as 8 bit, but may only be HXB_TRUE or HXB_FALSE
#define HXB_DTYPE_UINT8     0x02  // Unsigned 8 bit integer
//TODO int16 (power consumption), maybe float, fixed length string

// Defines for error codes
#define HXB_ERR_UNKNOWNVID    0x01  // A request for a VID which does not exist on the device was received
#define HXB_ERR_WRITEREADONLY 0x02  // A SETVALUE was received for a read-only value
#define HXB_ERR_CRCFAILED     0x03  // A packet failed the CRC check - TODO how can we find out what infomration was lost?
#define HXB_ERR_DATATYPE      0x04  // A setvalue packet was received, but the datatype of the packet does not fit the value to be set.

#endif
