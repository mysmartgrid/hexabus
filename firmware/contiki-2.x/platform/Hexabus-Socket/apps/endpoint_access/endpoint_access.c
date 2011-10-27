// Abstraction layer - transforms generic endpoint access to specific hardware function calls
// ============================================================================
// Hot to add a new endpoint
// - Add a case for your endpoint ID in endpoint_get_datatype, return the
//   datatype of your endpoint
// - Add a case for your endpoint ID in endpoint_write. This is executed when
//   someone sends a WRITE packet for your endpoint, or the state machine
//   has a switching rule that changes your endpoint's value. Feel free to use
//   events, function calls, or whatever. If your operation is too complicated,
//   put it in a function somewhere else and call it from here, so that the list
//   of  endpoint IDs doesn't get too cluttered. You can also use an error code
//   if the write fails. The state machine will recognize this (TODO to be implemented).
//   Make sure to check the DATATYPE before actually executing the WRITE ;)
// - Add a case for your endpoint ID in endpoint_read. The code that's already
//   there should pretty much explain how that's done.

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include "endpoint_access.h"

uint8_t endpoint_get_datatype(uint8_t eid) // returns the datatype of the endpoint, 0 if endpoint does not exist
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

uint8_t endpoint_write(uint8_t eid, struct hxb_value* value) // write access to an endpoint - returns 0 if okay, or some error code definde in hxb_packet.h
// TODO documentation: If we need something more complicated than "relay_on", how is it done? (Add new function into this file, execute, wait for return, ...
{
  PRINTF("endpoint_access: Set %d to %d, datatype %d.\r\n", eid, value->int8, value->datatype);
  switch(eid)
  {
    case 0:   // Endpoint 0: Device descriptor
      return HXB_ERR_WRITEREADONLY;
    case 1:   // Endpoint 1: Power switch on Hexabus Socket.
      if(value->datatype == HXB_DTYPE_BOOL)
      {
        if(value->int8 == HXB_TRUE)
        {
          relay_off();  // Note that the relay is connected in normally-closed position, so relay_off turns the power on and vice-versa
        } else {
          relay_on();
        }
        return 0;
      } else {
        return HXB_ERR_DATATYPE;
      }
    case 2:   // Endpoint 2: Power metering on Hexabus Socket -- read-only
      return HXB_ERR_WRITEREADONLY;
    default:  // Default: Endpoint does not exist
      return HXB_ERR_UNKNOWNVID;
  }
}

void endpoint_read(uint8_t eid, struct hxb_value* val) // read access to an endpoint
{
  switch(eid)
  {
    case 0:   // Endpoint 0: Hexabus device descriptor
      val->datatype = HXB_DTYPE_UINT32;
      val->int32 = 0x07;
      break;
    case 1:   // Endpoint 1: Hexabus Socket power switch
      val->datatype = HXB_DTYPE_BOOL;
      val->int8 = relay_get_state() == 0 ? HXB_TRUE : HXB_FALSE;
      break;
    case 2:   // Endpoint 2: Hexabus Socket power metering
      val->datatype = HXB_DTYPE_UINT8; // TODO needs more bits
      val->int8 = metering_get_power();
      break;
    default:
      val->datatype = HXB_DTYPE_UNDEFINED;
  }
}

