#ifndef HEXABUS_DEFINITIONS_H_
#define HEXABUS_DEFINITIONS_H_

// ======================================================================
// Definitions for fields

#define HXB_PORT              61616
#define HXB_GROUP             "ff05::205"                         // keep these in sync. the second define is used in
#define HXB_GROUP_RAW         0xff05, 0, 0, 0, 0, 0, 0, 0x0205    // hexaplug code.
#define HXB_HEADER            "HX0C"

#define HXB_STRING_PACKET_BUFFER_LENGTH 127
#define HXB_65BYTES_PACKET_BUFFER_LENGTH 65
#define HXB_16BYTES_PACKET_BUFFER_LENGTH 16

#endif
