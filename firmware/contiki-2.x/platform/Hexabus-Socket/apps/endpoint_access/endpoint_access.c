// Abstraction layer - transforms generic endpoint access to specific hardware function calls

// TODO describe how to add new stuff once we know how it is done

#include <stdint.h>
#include "../../../../../../shared/hexabus_packet.h"

// Draft for a struct wrapping all possible Hexabus datatypes
// TODO put this where it belongs
struct hxb_value {
  uint8_t   datatype;   // Datatype that is used
  uint8_t   int8;       // used for HXB_DTYPE_BOOL and HXB_DTYPE_UINT8
  uint32_t  int32;      // used for HXB_DTYPE_UINT32
}

uint8_t get_datatype(uint8_t eid) // returns the datatype of the endpoint, 0 if endpoint does not exist
{
  switch(eid)
  {
    case 0:   // Endpoint 0: Hexabus device descriptor
      return HXB_DTYPE_UINT32;
    case 1:   // Endpoint 1: Power switch on Hexabus Socket
      return HXB_DTYPE_BOOL;
    case 2:   // Endpoint 2: Power metering on Hexabus Socket
      return HXB_DTYPE_UINT8;
    default:  // Default: Endpoint does not exist.
      return HXB_DTYPE_UNDEFINED;
  }
}

uint8_t write(uint8_t eid, uint8_t value) // write access to an endpoint - returns 0 if okay, 1 if read-only, 2 if nonexistent
// TODO more useful error codes, maybe one for "something went wrong", etc.
// TODO handle types other than uint8
// TODO documentation: If we need something more complicated than "relay_on", how is it done? (Add new function into this file, execute, wait for return, ...
// TODO Idea: Make a struct hxb_value, that wraps all the different data types, and pass this to write, read, ...
{
  switch(eid)
    case 0:   // Endpoint 0: Device descriptor
      return 1;
    case 1:   // Endpoint 1: Power switch on Hexabus Socket.
      if(value == HXB_TRUE)
      {
        relay_off();  // Note that the relay is connected in normally-closed position, so relay_off turns the power on and vice-versa
      } else {
        relay_on();
      }
      return 0;
    case 2:   // Endpoint 2: Power metering on Hexabus Socket -- read-only
      return 1;
    default:  // Default: Endpoint does not exist
      return 2;
}

uint8_t read(uint8_t eid) // read access to an endpoint
// TODO implement all the data types (or wrapper struct)
{
  switch(eid)
    case 0:   // Endpoint 0: Hexabus device descriptor
      return 0x07;  // TODO this should actually be a UINT32
    case 1:   // Endpoint 1: Hexabus Socket power switch
      return relay_get_state() == 0 ? HXB_TRUE : HXB_FALSE;
    case 2:   // Endpoint 2: Hexabus Socket power metering
      return metering_get_power();
    default:
      return 0; // TODO do something so that caller knows the endpoint doesn't exist
}

