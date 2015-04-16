#ifndef HEXABUS_TYPES_H_
#define HEXABUS_TYPES_H_

#ifdef __cplusplus
#include <string>

namespace hexabus {
#endif

// Packet types
enum hxb_packet_type {
	HXB_PTYPE_ERROR          = 0x00, // An error occured -- check the error code field for more information
	HXB_PTYPE_INFO           = 0x01, // Endpoint provides information
	HXB_PTYPE_QUERY          = 0x02, // Endpoint is requested to provide information
	HXB_PTYPE_REPORT         = 0x03, // Endpoint provides information with seqence number of the causing QUERY
	HXB_PTYPE_WRITE          = 0x04, // Endpoint is requested to set its value
	HXB_PTYPE_EPINFO         = 0x09, // Endpoint metadata
	HXB_PTYPE_EPQUERY        = 0x0A, // Request endpoint metadata
	HXB_PTYPE_EPREPORT       = 0x0B, // Endpoint metadata with seqence number of the causing EPQUERY
	HXB_PTYPE_ACK            = 0x10, // Acknowledgement for INFO, EPINFO and PINFO packets
	HXB_PTYPE_PINFO          = 0x11, // Endpoint provides information with IP address of causing node
	HXB_PTYPE_EP_PROP_QUERY  = 0x13, // Endpoint is requested to provide information about its properties
	HXB_PTYPE_EP_PROP_REPORT = 0x14, // Endpoint provides property with seqence number of the causing QUERY
	HXB_PTYPE_EP_PROP_WRITE  = 0x15, // Endpoint is requested to write a property
};

// Data types
enum hxb_datatype {
	HXB_DTYPE_BOOL      = 0x00, // Boolean. Value is still represented by 8 bits, but may only be 0 or 1
	HXB_DTYPE_UINT8     = 0x01, // Unsigned 8 bit integer
	HXB_DTYPE_UINT16    = 0x02, // Unsigned 16 bit integer
	HXB_DTYPE_UINT32    = 0x03, // Unsigned 32 bit integer
	HXB_DTYPE_UINT64    = 0x04, // Unsigned 64 bit integer
	HXB_DTYPE_SINT8     = 0x05, // Signed 8 bit integer
	HXB_DTYPE_SINT16    = 0x06, // Signed 16 bit integer
	HXB_DTYPE_SINT32    = 0x07, // Signed 32 bit integer
	HXB_DTYPE_SINT64    = 0x08, // Signed 64 bit integer, mainly for unix dates
	HXB_DTYPE_FLOAT     = 0x09, // 32bit floating point
	HXB_DTYPE_128STRING = 0x0a, // 128char fixed length string
	HXB_DTYPE_65BYTES   = 0x0b, // raw 65 byte array, e.g. state machine data.
	HXB_DTYPE_16BYTES   = 0x0c, // raw 16 byte array, e.g. state machine ID.

	HXB_DTYPE_UNDEFINED = 0xFF, // Undefined: Nonexistent data type
};

enum hxb_flags {
	HXB_FLAG_NONE					= 0x00, // No flags set
	HXB_FLAG_WANT_ACK			= 0x01, // sending node requires acknowledgement
	HXB_FLAG_WANT_UL_ACK	= 0x02, // additonal flag for state machine changes
	HXB_FLAG_RELIABLE			= 0x04, // packet was sent through reliability layer
	HXB_FLAG_CONN_RESET		= 0x08, // indecates that the sender has reset the connection
};

// Error codes
enum hxb_error_code {
	HXB_ERR_SUCCESS        = 0x00, // reserved: No error
	HXB_ERR_UNKNOWNEID     = 0x01, // A request for an endpoint which does not exist on the device was received
	HXB_ERR_WRITEREADONLY  = 0x02, // A WRITE was received for a readonly endpoint
	HXB_ERR_DATATYPE       = 0x04, // A packet with a datatype that does not fit the endpoint was received
	HXB_ERR_INVALID_VALUE  = 0x05, // A value was encountered that cannot be interpreted

	// internal error values
	HXB_ERR_INTERNAL         = 0x80, // this is just a threshold. everything above is considered internal to a device and should never reach the network

	HXB_ERR_MALFORMED_PACKET  = 0x80,
	HXB_ERR_UNEXPECTED_PACKET = 0x81,
	HXB_ERR_NO_VALUE          = 0x82,
	HXB_ERR_INVALID_WRITE     = 0x83,
	HXB_ERR_OUT_OF_MEMORY     = 0x84
};

// State machine runtime states
enum STM_state_t {
  STM_STATE_STOPPED = 0,
  STM_STATE_RUNNING = 1
};

// Struct for passing Hexabus values around
// One struct for all data types (except blobs, because that'd need too much memory), with a datatype flag indicating which
// of the values is used. Used for passing values to and from endpoint_access
struct hxb_value {
	uint8_t               datatype;   // Datatype that is used, or HXB_DTYPE_UNDEFINED
	union {
		uint8_t   v_bool;
		uint8_t   v_u8;
		uint16_t  v_u16;
		uint32_t  v_u32;
		uint64_t  v_u64;
		int8_t    v_s8;
		int16_t   v_s16;
		int32_t   v_s32;
		int64_t   v_s64;
		float     v_float;
		char*     v_string;
		char*     v_binary;
	};
} __attribute__((packed));

#ifdef __cplusplus
inline std::string datatypeName(hxb_datatype type)
{
	switch (type) {
	case HXB_DTYPE_BOOL: return "Bool";
	case HXB_DTYPE_UINT8: return "UInt8";
	case HXB_DTYPE_UINT16: return "UInt16";
	case HXB_DTYPE_UINT32: return "UInt32";
	case HXB_DTYPE_UINT64: return "UInt64";
	case HXB_DTYPE_SINT8: return "Int8";
	case HXB_DTYPE_SINT16: return "Int16";
	case HXB_DTYPE_SINT32: return "Int32";
	case HXB_DTYPE_SINT64: return "Int64";
	case HXB_DTYPE_FLOAT: return "Float";
	case HXB_DTYPE_128STRING: return "String";
	case HXB_DTYPE_65BYTES: return "Binary(65)";
	case HXB_DTYPE_16BYTES: return "Binary(16)";

	case HXB_DTYPE_UNDEFINED: return "(undefined)";
	}

	return "(unknown)";
}

}
#endif

#endif
