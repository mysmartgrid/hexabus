#ifndef HEXABUS_VALUE_H_
#define HEXABUS_VALUE_H_

// Struct for passing Hexabus values around
// One struct for all data types (except 128string, because that'd need too much memory), with a datatype flag indicating which
// of the values is used. Used for passing values to and from
// endpoint_access
struct hxb_value {
  uint8_t   datatype;   // Datatype that is used, or HXB_DTYPE_UNDEFINED
  char      data[8];    // leave enough room for the largest datatype (datetime in this case)
}; //TODO can't we use a union for this?

#endif
