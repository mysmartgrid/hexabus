#ifndef HEXABUS_DEFINITIONS_H_
#define HEXABUS_DEFINITIONS_H_

// ======================================================================
// Definitions for fields

#define HXB_PORT              61616
#define HXB_GROUP             "ff05::114"                         // keep these in sync. the second define is used in
#define HXB_GROUP_RAW         0xff05, 0, 0, 0, 0, 0, 0, 0x0114    // hexaplug code.
#define HXB_HEADER            "HX0D"

// Boolean values
#define HXB_TRUE              1
#define HXB_FALSE             0
// TODO Do we need a "toggle"?

// Packet types
#define HXB_PTYPE_ERROR       0x00  // An error occured -- check the error code field for more information
#define HXB_PTYPE_INFO        0x01  // Endpoint provides information
#define HXB_PTYPE_QUERY       0x02  // Endpoint is requested to provide information
#define HXB_PTYPE_WRITE       0x04  // Endpoint is requested to set its value
#define HXB_PTYPE_EPINFO      0x09  // Endpoint metadata
#define HXB_PTYPE_EPQUERY     0x0A  // Request endpoint metadata

// Flags
#define HXB_FLAG_CONFIRM      0x01  // Requests an acknowledgement

// Data types
#define HXB_DTYPE_UNDEFINED   0x00  // Undefined: Nonexistent data type
#define HXB_DTYPE_BOOL        0x01  // Boolean. Value still represented by 8 bits, but may only be HXB_TRUE or HXB_FALSE
#define HXB_DTYPE_UINT8       0x02  // Unsigned 8 bit integer
#define HXB_DTYPE_UINT32      0x03  // Unsigned 32 bit integer
#define HXB_DTYPE_DATETIME    0x04  // Date and time
#define HXB_DTYPE_FLOAT       0x05  // 32bit floating point
#define HXB_DTYPE_128STRING   0x06  // 128char fixed length string
#define HXB_DTYPE_TIMESTAMP   0x07  // timestamp - used for measuring durations, time differences and so on - uint32; seconds
#define HXB_DTYPE_66BYTES     0x08  // raw 66 byte array, e.g. state machine.


// Error codes
//                            0x00     reserved: No error
#define HXB_ERR_UNKNOWNEID    0x01  // A request for an endpoint which does not exist on the device was received
#define HXB_ERR_WRITEREADONLY 0x02  // A WRITE was received for a readonly endpoint
#define HXB_ERR_CRCFAILED     0x03  // A packet failed the CRC check -- TODO How can we find out what information was lost?
#define HXB_ERR_DATATYPE      0x04  // A packet with a datatype that does not fit the endpoint was received
#define HXB_ERR_INVALID_VALUE 0x05  // A value was encountered that cannot be interpreted
#define HXB_ERR_RESEND_REQUEST 0x06 // The packet with the seqence number in the sequence number filed was lost

// Operators for comparison in state machine
#define STM_EQ                0x00
#define STM_LEQ               0x01
#define STM_GEQ               0x02
#define STM_LT                0x03
#define STM_GT                0x04
#define STM_NEQ               0x05

// State machine runtime states
enum STM_state_t {
  STM_STATE_STOPPED = 0,
  STM_STATE_RUNNING = 1};



#endif
