// Abstraction layer - transforms generic endpoint access to specific hardware function calls

#include "endpoint_access.h"

// TODO describe how to add new stuff once we know how it is done

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
  printf("endpoint_access: Set %d to %d, datatype %d.\r\n", eid, value->int8, value->datatype);
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

