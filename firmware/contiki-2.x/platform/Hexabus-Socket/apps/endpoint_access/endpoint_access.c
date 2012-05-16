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
#include "humidity.h"
#include "pressure.h"
#include "ir_receiver.h"
#include "endpoints.h"

uint8_t endpoint_get_datatype(uint8_t eid) // returns the datatype of the endpoint, 0 if endpoint does not exist
{
  switch(eid)
  {
    case EP_DEVICE_DESCRIPTOR_EID:   // Endpoint 0: Hexabus device descriptor
      return HXB_DTYPE_UINT32;
    case EP_POWER_SWITCH_EID:   // Endpoint 1: Power switch on Hexabus Socket
      return HXB_DTYPE_BOOL;
    case EP_POWER_METER_EID:   // Endpoint 2: Power metering on Hexabus Socket
      return HXB_DTYPE_UINT32;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE_EID:   // Endpoint 3: Temperature value
      return HXB_DTYPE_FLOAT;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON_EID:
      return HXB_DTYPE_BOOL;
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY_EID:
      return HXB_DTYPE_FLOAT;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE_EID:
      return HXB_DTYPE_PRESSURE;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR_EID:
      return HXB_DTYPE_FLOAT;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER_EID:
      return HXB_DTYPE_UINT8;
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_EID_PRESSED:
    case EP_HEXAPUSH_EID_CLICKED:
      return HXB_DTYPE_UINT8;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR_EID:
      return HXB_DTYPE_UINT8;
#endif
#if HEXONOFF_ENABLE
    case EP_HEXONOFF_EID_SET:
    case EP_HEXONOFF_EID_TOGGLE:
      return HXB_DTYPE_UINT8;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD_EID:
      return HXB_DTYPE_FLOAT;
#endif
#if IR_RECEIVER_ENABLE
    case EP_IR_RECEIVER_EID:
      return HXB_DTYPE_UINT32;
#endif
    default:  // Default: Endpoint does not exist.
      return HXB_DTYPE_UNDEFINED;
  }
}

void endpoint_get_name(uint8_t eid, char* buffer)  // writes the name of the endpoint into the buffer. Max 127 chars.
{
  // fill buffer with \0
  int i;
  for(i = 0; i < HXB_STRING_PACKET_MAX_BUFFER_LENGTH; i++)
    buffer[i] = '\0';
  switch(eid)
  {
    case EP_DEVICE_DESCRIPTOR_EID:
      strncpy(buffer, "Hexabus Socket - Development Version", HXB_STRING_PACKET_MAX_BUFFER_LENGTH); // TODO make this consistent with the MDNS name
      break;
    case EP_POWER_SWITCH_EID:
      strncpy(buffer, "Main Switch", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_POWER_METER_EID:
      strncpy(buffer, "Power Meter", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE_EID:
      strncpy(buffer, "Temperature Sensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON_EID:
      strncpy(buffer, "Hexabus Socket Pushbutton", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY_EID:
      strncpy(buffer, "Humidity sensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE_EID:
      strncpy(buffer, "Barometric pressure sensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR_EID:
      strncpy(buffer, "Lightsensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER_EID:
      strncpy(buffer, "Window Shutter", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_EID_PRESSED:
      strncpy(buffer, "Pressed Hexapush buttons", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_HEXAPUSH_EID_CLICKED:
      strncpy(buffer, "Clicked Hexapush buttons", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR_EID:
      strncpy(buffer, "Presence Detector", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if HEXONOFF_ENABLE
    case EP_HEXONOFF_EID_SET:
      strncpy(buffer, "Hexonoff, your friendly output setter.", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_HEXONOFF_EID_TOGGLE:
      strncpy(buffer, "Hexonoff, your friendly output toggler.", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD_EID:
      strncpy(buffer, "Analog reader", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if IR_RECEIVER_ENABLE
    case EP_IR_RECEIVER_EID:
      strncpy(buffer, "IR remote control receiver", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
    default:
      buffer[0] = '\0'; // put the empty String in the buffer (set first character to \0)
      break;
  }
  buffer[HXB_STRING_PACKET_MAX_BUFFER_LENGTH] = '\0'; // Set last character to \0 in case some string was too long
}

uint8_t endpoint_write(uint8_t eid, struct hxb_value* value) // write access to an endpoint - returns 0 if okay, or an error code as defined in hxb_packet.h
{
  switch(eid)
  {
    case EP_DEVICE_DESCRIPTOR_EID:   // Endpoint 0: Device descriptor
      return HXB_ERR_WRITEREADONLY;
    case EP_POWER_SWITCH_EID:   // Endpoint 1: Power switch on Hexabus Socket.
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
    case EP_POWER_METER_EID:   // Endpoint 2: Power metering on Hexabus Socket -- read-only
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE_EID:   // Endpoint 3: Temperature value on Hexabus Socket -- read-only
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON_EID:
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY_EID:
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE_EID:
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD_EID:
#endif
      return HXB_ERR_WRITEREADONLY;
#if SHUTTER_ENABLE
    case EP_SHUTTER_EID:
      if(value->datatype == HXB_DTYPE_UINT8) {
          shutter_toggle(*(uint8_t*)&value->data);
          return 0;
      } else {
          return HXB_ERR_DATATYPE;
      }
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_EID_PRESSED:
    case EP_HEXAPUSH_EID_CLICKED:
      return HXB_ERR_WRITEREADONLY;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR_EID:
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
    case EP_HEXONOFF_EID_SET:
        if(value->datatype == HXB_DTYPE_UINT8) {
            set_outputs(*(uint8_t*)&value->data);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
    case EP_HEXONOFF_EID_TOGGLE:
        if(value->datatype == HXB_DTYPE_UINT8) {
            toggle_outputs(*(uint8_t*)&value->data);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR_EID:
      return HXB_ERR_WRITEREADONLY;
#endif
    default:  // Default: Endpoint does not exist
      return HXB_ERR_UNKNOWNEID;
  }
}

void endpoint_read(uint8_t eid, struct hxb_value* val) // read access to an endpoint
{
  switch(eid)
  {
    /* -========== Endpoint 0: Hexabus Device Descriptor ==================- */
    case EP_DEVICE_DESCRIPTOR_EID:
      val->datatype = HXB_DTYPE_UINT32;
      *(uint32_t*)&val->data = 0;       // start out with zero.
      // endpoints 1 and 2 are not optional
      *(uint32_t*)&val->data += 1 << (EP_POWER_SWITCH_EID - 1); // shifted EID minus one so that Bit#0 corresponds to EID 1, Bit#1 to EID 2, ...
      *(uint32_t*)&val->data += 1 << (EP_POWER_METER_EID - 1);
#if TEMPERATURE_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_TEMPERATURE_EID - 1);
#endif
#if BUTTON_HAS_EID
      *(uint32_t*)&val->data += 1 << (EP_BUTTON_EID - 1);
#endif
#if HUMIDITY_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_HUMIDITY_EID - 1);
#endif
#if PRESSURE_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_PRESSURE_EID - 1);
#endif
#if ANALOGREAD_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_ANALOGREAD_EID - 1);
#endif
#if SHUTTER_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_SHUTTER_EID - 1);
#endif
#if HEXAPUSH_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_HEXAPUSH_EID_PRESSED - 1);
      *(uint32_t*)&val->data += 1 << (EP_HEXAPUSH_EID_CLICKED - 1);
#endif
#if PRESENCE_DETECTOR_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_PRESENCE_DETECTOR_EID - 1);
#endif
#if HEXONOFF_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_HEXONOFF_EID_SET - 1);
      *(uint32_t*)&val->data += 1 << (EP_HEXONOFF_EID_TOGGLE - 1);
#endif
#if LIGHTSENSOR_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_LIGHTSENSOR_EID - 1);
#endif
#if IR_RECEIVER_ENABLE
      *(uint32_t*)&val->data += 1 << (EP_IR_RECEIVER_EID - 1);
#endif
      break;
    /* -==================================================================- */

    case EP_POWER_SWITCH_EID:   // Endpoint 1: Hexabus Socket power switch
      val->datatype = HXB_DTYPE_BOOL;
      *(uint8_t*)&val->data = relay_get_state() == 0 ? HXB_TRUE : HXB_FALSE;
      break;
    case EP_POWER_METER_EID:   // Endpoint 2: Hexabus Socket power metering
      val->datatype = HXB_DTYPE_UINT32;
      *(uint32_t*)&val->data = metering_get_power();
      break;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE_EID:   // Endpoint 3: Hexabus temperaure metering
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = temperature_get();
      break;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON_EID:   // Endpoint 4: Pushbutton on the Hexabus-Socket
      val->datatype = HXB_DTYPE_BOOL;
      *(uint8_t*)&val->data = button_get_pushed();
      break;
#endif
#if HUMIDITY_ENABLE
    case HUMIDITY_EID:
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = read_humidity();
      break;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE_EID:
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = read_pressure();
      break;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR_EID:
        val->datatype = HXB_DTYPE_FLOAT;
        *(float*)&val->data = get_lightvalue();
        break;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER_EID:
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = shutter_get_state();
      break;
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_EID_PRESSED:  //Pressed und released
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = get_buttonstate();
      break;
    case EP_HEXAPUSH_EID_CLICKED: //Clicked
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = get_clickstate();
      break;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR:
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = is_presence();
      break;
#endif
#if HEXONOFF_ENABLE
    case EP_HEXONOFF_EID_SET:
    case EP_HEXONOFF_EID_TOGGLE:
        val->datatype = HXB_DTYPE_UINT8;
        *(uint8_t*)&val->data = get_outputs();
        break;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD_EID:
        val->datatype = HXB_DTYPE_FLOAT;
        *(float*)&val->data = get_analogvalue();
        break;
#endif
#if IR_RECEIVER_ENABLE
    case EP_IR_RECEIVER_EID:
        val->datatype = HXB_DTYPE_UINT32;
        *(uint32_t*)&val->data = ir_get_last_command();
        break;
#endif
    default:
      val->datatype = HXB_DTYPE_UNDEFINED;
  }
}

