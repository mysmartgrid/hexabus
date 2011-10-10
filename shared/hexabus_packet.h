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
#define HXB_PTYPE_ERROR     0x00  // An error occured, check the errorcode for more information
#define HXB_PTYPE_BROADCAST 0x01  // Broadcast of a sensor value, either following a REQVALUE or periodically
#define HXB_PTYPE_SETVALUE  0x02  // Target device is requested to set [vid] := value
#define HXB_PTYPE_REQVALUE  0x03  // Target device is requested to broadcast the value with the ID vid
#define HXB_PTYPE_REPLY     0x04  // This is the reply to a REQVALUE - it looks the same as a BROADCAST, except ist not sent to a multicast address, but to the Device that sent the REQVALUE
//TODO HXB_PTYPE_ACK, HXB_PTYPE_RESET, HXB_PTYPE_HEARTBEAT -- geht alles als INFO

// Defines for flags
#define HXB_FLAG_IMPORTANT  0x01  // Packet should be ACKnowledged, otherwise it is retransmitted
#define HXB_FLAG_RETRANSMIT 0x02  // Packet is a retransmission

//TODO SET-AND-CONFIRM: Acknowledge the SETVALUE with a REPLY (or something new)

// Defines for datatypes
#define HXB_DTYPE_BOOL      0x01  // Boolean -- value is represented as 8 bit, but may only be HXB_TRUE or HXB_FALSE
#define HXB_DTYPE_UINT8     0x02  // Unsigned 8 bit integer
//TODO int16 (power consumption), maybe float, fixed length string

// Defines for error codes
#define HXB_ERR_UNKNOWNVID  0x01  // A request for a VID which does not exist on the device was received
#define HXB_ERR_SETREADONLY 0x02  // A SETVALUE was received for a read-only value
#define HXB_ERR_CRCFAILED   0x03  // A packet failed the CRC check - TODO how can we find out what infomration was lost?
#define HXB_ERR_DATATYPE    0x04  // A setvalue packet was received, but the datatype of the packet does not fit the value to be set.

#endif
