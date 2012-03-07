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
#include "button.h"
#include "analogread.h"
#include "ir_receiver.h"

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
#if BUTTON_HAS_EID
    case 4:
      return HXB_DTYPE_BOOL;
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
      return HXB_DTYPE_UINT8;
#endif
#if HEXONOFF_ENABLE
    case 27:
    case 28:
      return HXB_DTYPE_UINT8;
#endif
#if ANALOGREAD_ENABLE
    case ANALOGREAD_EID:
      return HXB_DTYPE_FLOAT;
#endif
#if IR_RECEIVER_ENABLE
    case 30:
      return HXB_DTYPE_UINT32;
#endif
#if IR_SERVO_ENABLE
    case 31:
      return HXB_DTYPE_UINT8;
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
#if TEMPERATURE_ENABLE
    case 3:
      strncpy(buffer, "Temperature Sensor", 127);
      break;
#endif
#if BUTTON_HAS_EID
    case 4:
      strncpy(buffer, "Hexabus Socket Pushbutton", 127);
      break;
#endif
#if SHUTTER_ENABLE
    case 23:
      strncpy(buffer, "Window Shutter", 127);
      break;
#endif
#if HEXAPUSH_ENABLE
    case 24:
      strncpy(buffer, "Pressed Hexapush buttons", 127);
      break;
    case 25:
      strncpy(buffer, "Clicked Hexapush buttons", 127);
      break;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case 26:
      strncpy(buffer, "Presence Detector", 127);
      break;
#endif
#if HEXONOFF_ENABLE
    case 27:
      strncpy(buffer, "Hexonoff, your friendly output setter.", 127);
      break;
    case 28:
      strncpy(buffer, "Hexonoff, your friendly output toggler.", 127);
      break;
#endif
#if ANALOGREAD_ENABLE
    case ANALOGREAD_EID:
      strncpy(buffer, "Analog reader", 127);
      break;
#endif
#if IR_RECEIVER_ENABLE
    case 30:
      strncpy(buffer, "IR remote control receiver", 127);
      break;
#endif
#if IR_SERVO_ENABLE
    case 31:
      strncpy(buffer, "Servo controller", 127);
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
#if BUTTON_HAS_EID
    case 4:
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
#if PRESENCE_DETECTOR_ENABLE
    case 26:
      if(value->datatype == HXB_DTYPE_UINT8)
      {
        if(*(uint8_t*)&value->data == 1)
        {
            global_presence_detected();
        } else if(*(uint8_t*)&value->data == 0) {
            no_raw_presence();
        } else {
            raw_presence_detected();
        }
        return 0;
      } else {
        return HXB_ERR_DATATYPE;
      }
#endif
#if HEXONOFF_ENABLE
    case 27:
        if(value->datatype == HXB_DTYPE_UINT8) {
            set_outputs(*(uint8_t*)&value->data);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
    case 28:
        if(value->datatype == HXB_DTYPE_UINT8) {
            toggle_outputs(*(uint8_t*)&value->data);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
#endif
#if IR_SERVO_ENABLE
    case 31:
        if(value->datatype == HXB_DTYPE_UINT8) {
            set_servo(*(uint8_t*)&value->data);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
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
      *(uint32_t*)&val->data = 0x03;    // 0x07: 0..00011: Enpoints 1 and 2 exist.
#if TEMPERATURE_ENABLE
      *(uint32_t*)&val->data += 0x04;      // +4: Endpoint 3 also exists
#endif
#if BUTTON_HAS_EID
      *(uint32_t*)&val->data += 0x08; // +8: Endpoint 4 also exists
#endif
#if SHUTTER_ENABLE
      *(uint32_t*)&val->data += 0x00400000; // set bit #22 for EID 23
#endif
#if HEXAPUSH_ENABLE
      *(uint32_t*)&val->data += 0x01800000; // set bits 23 and 24 for EIDs 24 and 25
#endif
#if PRESENCE_DETECTOR_ENABLE
      *(uint32_t*)&val->data += 0x02000000; // set bit 25 for EID 26
#endif
#if HEXONOFF_ENABLE
      *(uint32_t*)&val->data += 0x0C000000; // set bits 26 and 27 for EIDs 27 and 28
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
#if BUTTON_HAS_EID
    case 4:   // Endpoint 4: Pushbutton on the Hexabus-Socket
      val->datatype = HXB_DTYPE_BOOL;
      *(uint8_t*)&val->data = button_get_pushed();
      break;
#endif
#if SHUTTER_ENABLE
    case 23:
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = shutter_get_state();
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
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = is_presence();
      break;
#endif
#if HEXONOFF_ENABLE
    case 27:
    case 28:
        val->datatype = HXB_DTYPE_UINT8;
        *(uint8_t*)&val->data = get_outputs();
        break;
#endif
#if ANALOGREAD_ENABLE
    case ANALOGREAD_EID:
        val->datatype = HXB_DTYPE_FLOAT;
        *(float*)&val->data = get_analogvalue();
        break;
#endif
#if IR_RECEIVER_ENABLE
    case 30:
        val->datatype = HXB_DTYPE_UINT32;
        *(uint32_t*)&val->data = ir_get_last_command();
        break;
#endif
#if IR_SERVO_ENABLE
    case 31:
        val->datatype = HXB_DTYPE_UINT8;
        *(uint8_t*)&val->data = get_servo();
        break;
#endif
    default:
      val->datatype = HXB_DTYPE_UNDEFINED;
  }
}

