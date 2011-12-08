// Abstraction layer - transforms generic endpoint access to specific hardware function calls
// ============================================================================

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include "hexabus_config.h"
#include "endpoint_access.h"
#include "temperature.h"

uint8_t endpoint_get_datatype(uint8_t eid) // returns the datatype of the endpoint, 0 if endpoint does not exist
{
  switch(eid)
  {
    case 0:   // Endpoint 0: Hexabus device descriptor
      return HXB_DTYPE_UINT32;
    case 1:   // Endpoint 1: Power switch on Hexabus Socket
      return HXB_DTYPE_BOOL;
    case 2:   // Endpoint 2: Power metering on Hexabus Socket
      return HXB_DTYPE_UINT32;
#if TEMPERATURE_ENABLE
    case 3:   // Endpoint 3: Temperature value 
      return HXB_DTYPE_FLOAT;
#endif
#if SHUTTER_ENABLE
    case 23:
      return HXB_DTYPE_UINT8;
#endif
#if HEXAPUSH_ENABLE
    case 24:
    case 25:
      return HXB_DTYPE_UINT8;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case 26:
      return HXB_DTYPE_BOOL;
#endif
    default:  // Default: Endpoint does not exist.
      return HXB_DTYPE_UNDEFINED;
  }
}

void endpoint_get_name(uint8_t eid, char* buffer)  // writes the name of the endpoint into the buffer. Max 127 chars.
{
  // fill buffer with \0
  int i;
  for(i = 0; i < 127; i++)
    buffer[i] = '\0';
  switch(eid)
  {
    case 0:
      strncpy(buffer, "Hexabus Socket - Development Version", 127); // TODO make this consistent with the MDNS name
      break;
    case 1:
      strncpy(buffer, "Main Switch", 127);
      break;
    case 2:
      strncpy(buffer, "Power Meter", 127);
      break;
#ifdef TEMPERATURE_ENABLE
    case 3:
      strncpy(buffer, "Temperature Sensor", 127);
      break;
#endif
    default:
      buffer[0] = '\0'; // put the empty String in the buffer (set first character to \0)
      break;
  }
  buffer[127] = '\0'; // Set last character to \0 in case some string was too long
}

uint8_t endpoint_write(uint8_t eid, struct hxb_value* value) // write access to an endpoint - returns 0 if okay, or some error code definde in hxb_packet.h
{
  // PRINTF("endpoint_access: Set %d to %d, datatype %d.\r\n", eid, (uint32_t)(value->float32*1000), value->datatype); TODO - print all the datatypes
  switch(eid)
  {
    case 0:   // Endpoint 0: Device descriptor
      return HXB_ERR_WRITEREADONLY;
    case 1:   // Endpoint 1: Power switch on Hexabus Socket.
      if(value->datatype == HXB_DTYPE_BOOL)
      {
        if(*(uint8_t*)&value->data == HXB_TRUE)
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
#if TEMPERATURE_ENABLE
    case 3:   // Endpoint 3: Temperature value on Hexabus Socket -- read-only
#endif
      return HXB_ERR_WRITEREADONLY;
#if SHUTTER_ENABLE
    case 23:
      if(value->datatype == HXB_DTYPE_UINT8) {
          shutter_toggle(*(uint8_t*)&value->data);
          return 0;
      } else {
          return HXB_ERR_DATATYPE;
      }
#endif
    default:  // Default: Endpoint does not exist
      return HXB_ERR_UNKNOWNEID;
  }
}

void endpoint_read(uint8_t eid, struct hxb_value* val) // read access to an endpoint
{
  switch(eid)
  {
    case 0:   // Endpoint 0: Hexabus device descriptor
      val->datatype = HXB_DTYPE_UINT32;
      *(uint32_t*)&val->data = 0x07;    // 0x07: 0..00111: Enpoints 0, 1 and 2 exist.
#if TEMPERATURE_ENABLE
      *(uint32_t*)&val->data += 0x08;      // +8: Endpoint 3 also exists
#endif
      break;
    case 1:   // Endpoint 1: Hexabus Socket power switch
      val->datatype = HXB_DTYPE_BOOL;
      *(uint8_t*)&val->data = relay_get_state() == 0 ? HXB_TRUE : HXB_FALSE;
      break;
    case 2:   // Endpoint 2: Hexabus Socket power metering
      val->datatype = HXB_DTYPE_UINT32;
      *(uint32_t*)&val->data = metering_get_power();
      break;
#if TEMPERATURE_ENABLE
    case 3:   // Endpoint 3: Hexabus temperaure metering
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = temperature_get();
      break;
#endif
#if SHUTTER_ENABLE
    case 23:
      val->datatype = HXB_DTYPE_UINT8;
      val->*(uint8_t*)&value->data = shutter_get_state();
      break;
#endif
#if HEXAPUSH_ENABLE
    case 24:  //Pressed und released
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = get_buttonstate();
      break;
    case 25: //Clicked
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = get_clickstate();
      break;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case 26:
      val->datatype = HXB_DTYPE_BOOL;
      *(uint8_t*)&val->data = presence_active() == 0 ? HXB_FALSE : HXB_TRUE;
      break;
#endif
    default:
      val->datatype = HXB_DTYPE_UNDEFINED;
  }
}

