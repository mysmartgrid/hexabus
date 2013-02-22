// Abstraction layer - transforms generic endpoint access to specific hardware function calls
// ============================================================================

#include "hexabus_config.h"
#include "endpoint_access.h"
#include "temperature.h"
#include "button.h"
#include "analogread.h"
#include "humidity.h"
#include "pressure.h"
#include "ir_receiver.h"
#include "metering.h"
#include "endpoints.h"
#include "net/uip.h"
#include "packet_builder.h"
#include "udp_handler.h"
#include <stdbool.h>

#if ENDPOINT_ACCESS_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint8_t endpoint_get_datatype(uint32_t eid) // returns the datatype of the endpoint, 0 if endpoint does not exist
{
  switch(eid)
  {
    case EP_DEVICE_DESCRIPTOR:   // Endpoint 0: Hexabus device descriptor
      return HXB_DTYPE_UINT32;
    case EP_POWER_SWITCH:   // Endpoint 1: Power switch on Hexabus Socket
      return HXB_DTYPE_BOOL;
    case EP_POWER_METER:   // Endpoint 2: Power metering on Hexabus Socket
      return HXB_DTYPE_UINT32;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE:   // Endpoint 3: Temperature value
      return HXB_DTYPE_FLOAT;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON:
      return HXB_DTYPE_BOOL;
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY:
      return HXB_DTYPE_FLOAT;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE:
      return HXB_DTYPE_FLOAT;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR:
      return HXB_DTYPE_FLOAT;
#endif
#if METERING_ENERGY
    case EP_ENERGY_METER_TOTAL:
    case EP_ENERGY_METER:
      return HXB_DTYPE_FLOAT;
#endif
#if SM_UPLOAD_ENABLE
    case EP_SM_CONTROL:
      return HXB_DTYPE_UINT8;
    case EP_SM_UP_RECEIVER:
      return HXB_DTYPE_66BYTES;
    case EP_SM_UP_ACKNAK:
      return HXB_DTYPE_BOOL;
    case EP_SM_RESET_ID:
      return HXB_DTYPE_16BYTES;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER:
      return HXB_DTYPE_UINT8;
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_PRESSED:
    case EP_HEXAPUSH_CLICKED:
      return HXB_DTYPE_UINT8;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR:
      return HXB_DTYPE_UINT8;
#endif
#if HEXONOFF_ENABLE
    case EP_HEXONOFF_SET:
    case EP_HEXONOFF_TOGGLE:
      return HXB_DTYPE_UINT8;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD:
      return HXB_DTYPE_FLOAT;
#endif
#if IR_RECEIVER_ENABLE
    case EP_IR_RECEIVER:
      return HXB_DTYPE_UINT32;
#endif
    default:  // Default: Endpoint does not exist.
      return HXB_DTYPE_UNDEFINED;
  }
}

void endpoint_get_name(uint32_t eid, char* buffer)  // writes the name of the endpoint into the buffer. Max 127 chars.
{
  // fill buffer with \0
  int i;
  for(i = 0; i < HXB_STRING_PACKET_MAX_BUFFER_LENGTH; i++)
    buffer[i] = '\0';
  switch(eid)
  {
    case EP_DEVICE_DESCRIPTOR:
      strncpy(buffer, "Hexabus Socket - Development Version", HXB_STRING_PACKET_MAX_BUFFER_LENGTH); // TODO make this consistent with the MDNS name
      break;
    case EP_POWER_SWITCH:
      strncpy(buffer, "Main Switch", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_POWER_METER:
      strncpy(buffer, "Power Meter", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE:
      strncpy(buffer, "Temperature Sensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON:
      strncpy(buffer, "Hexabus Socket Pushbutton", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY:
      strncpy(buffer, "Humidity sensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE:
      strncpy(buffer, "Barometric pressure sensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR:
      strncpy(buffer, "Lightsensor", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
#endif
#if METERING_ENERGY
    case EP_ENERGY_METER_TOTAL:
      strncpy(buffer, "Energy Meter Total", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_ENERGY_METER:
      strncpy(buffer, "Energy Meter", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if SM_UPLOAD_ENABLE
    case EP_SM_CONTROL:
      strncpy(buffer, "Statemachine Control", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_SM_UP_RECEIVER:
      strncpy(buffer, "Statemachine Upload Receiver", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_SM_UP_ACKNAK:
      strncpy(buffer, "Statemachine Upload ACK/NAK", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_SM_RESET_ID:
      strncpy(buffer, "Statemachine emergency-reset endpoint.", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER:
      strncpy(buffer, "Window Shutter", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_PRESSED:
      strncpy(buffer, "Pressed Hexapush buttons", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_HEXAPUSH_CLICKED:
      strncpy(buffer, "Clicked Hexapush buttons", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR:
      strncpy(buffer, "Presence Detector", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if HEXONOFF_ENABLE
    case EP_HEXONOFF_SET:
      strncpy(buffer, "Hexonoff, your friendly output setter.", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
    case EP_HEXONOFF_TOGGLE:
      strncpy(buffer, "Hexonoff, your friendly output toggler.", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD:
      strncpy(buffer, "Analog reader", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
#if IR_RECEIVER_ENABLE
    case EP_IR_RECEIVER:
      strncpy(buffer, "IR remote control receiver", HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
      break;
#endif
    default:
      buffer[0] = '\0'; // put the empty String in the buffer (set first character to \0)
      break;
  }
  buffer[HXB_STRING_PACKET_MAX_BUFFER_LENGTH] = '\0'; // Set last character to \0 in case some string was too long
}

uint8_t endpoint_write(uint32_t eid, struct hxb_value* value) // write access to an endpoint - returns 0 if okay, or an error code as defined in hxb_packet.h
{
  switch(eid)
  {
    case EP_DEVICE_DESCRIPTOR:   // Endpoint 0: Device descriptor
      return HXB_ERR_WRITEREADONLY;
    case EP_POWER_SWITCH:   // Endpoint 1: Power switch on Hexabus Socket.
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
    case EP_POWER_METER:   // Endpoint 2: Power metering on Hexabus Socket -- read-only
      return HXB_ERR_WRITEREADONLY;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE:   // Endpoint 3: Temperature value on Hexabus Socket -- read-only
      return HXB_ERR_WRITEREADONLY;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON:
      return HXB_ERR_WRITEREADONLY;
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY:
      return HXB_ERR_WRITEREADONLY;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE:
      return HXB_ERR_WRITEREADONLY;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD:
      return HXB_ERR_WRITEREADONLY;
#endif
#if METERING_ENERGY
    case EP_ENERGY_METER_TOTAL:
      return HXB_ERR_WRITEREADONLY;
    case EP_ENERGY_METER:
      if(value->datatype == HXB_DTYPE_FLOAT)
      {
        metering_reset_energy();
        return 0;
      } else {
        return HXB_ERR_DATATYPE;
      }
#endif
#if SM_UPLOAD_ENABLE
    case EP_SM_CONTROL:
      if(value->datatype == HXB_DTYPE_UINT8) {
        if(*(uint8_t*)&value->data == STM_STATE_STOPPED ) {
          sm_stop();
        } else if(*(uint8_t*)&value->data == STM_STATE_RUNNING) {
          if (sm_is_running()) {
            sm_restart();
          } else {
            sm_start();
          }
        } else {
          return HXB_ERR_INVALID_VALUE;
        }
        return 0;
      } else {
        return HXB_ERR_DATATYPE;
      }
    case EP_SM_UP_RECEIVER:
      PRINTF("SM: Attempting to write new chunk to EEPROM\n");
      struct hxb_value val;
      val.datatype=HXB_DTYPE_BOOL;
      if(value->datatype == HXB_DTYPE_66BYTES) {
        char* payload = *(char**)&value->data;
        uint8_t chunk_id = (uint8_t)payload[0];
        if (sm_write_chunk(chunk_id, payload+1) == true) {
          // send ACK to SM_SENDER
          PRINTF("SENDING ACK");
          val.data[0] = HXB_TRUE;
          struct hxb_packet_int8 packet = make_value_packet_int8(EP_SM_UP_ACKNAK, &val);
          send_packet((char*)&packet, sizeof(packet));
          return 0;
        } else {
          // send NAK to SM_SENDER
          PRINTF("SENDING NAK");
          val.data[0] = HXB_FALSE;
          struct hxb_packet_int8 packet = make_value_packet_int8(EP_SM_UP_ACKNAK, &val);
          send_packet((char*)&packet, sizeof(packet));
          return HXB_ERR_INVALID_VALUE;
        }
      } else {
        // send NAK to SM_SENDER
        PRINTF("SENDING ERROR DATATYPE");
        val.data[0] = HXB_FALSE;
        struct hxb_packet_int8 packet = make_value_packet_int8(EP_SM_UP_ACKNAK, &val);
        send_packet((char*)&packet, sizeof(packet));
        return HXB_ERR_DATATYPE;
      }
      break;
    case EP_SM_UP_ACKNAK:
      return HXB_ERR_WRITEREADONLY;
    case EP_SM_RESET_ID:
      return HXB_ERR_WRITEREADONLY;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER:
      if(value->datatype == HXB_DTYPE_UINT8) {
          shutter_toggle(*(uint8_t*)&value->data);
          return 0;
      } else {
          return HXB_ERR_DATATYPE;
      }
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_PRESSED:
    case EP_HEXAPUSH_CLICKED:
      return HXB_ERR_WRITEREADONLY;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR:
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
    case EP_HEXONOFF_SET:
        if(value->datatype == HXB_DTYPE_UINT8) {
            set_outputs(*(uint8_t*)&value->data);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
    case EP_HEXONOFF_TOGGLE:
        if(value->datatype == HXB_DTYPE_UINT8) {
            toggle_outputs(*(uint8_t*)&value->data);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR:
      return HXB_ERR_WRITEREADONLY;
#endif
    default:  // Default: Endpoint does not exist
      return HXB_ERR_UNKNOWNEID;
  }
}

void endpoint_read(uint32_t eid, struct hxb_value* val) // read access to an endpoint
{
  switch(eid)
  {
    /* -========== Endpoint 0: Hexabus Device Descriptor ==================- */
    case EP_DEVICE_DESCRIPTOR:
      val->datatype = HXB_DTYPE_UINT32;
      *(uint32_t*)&val->data = 1UL;       // start out with last bit set: EID0 always exists
      // endpoints 1 and 2 are not optional
      *(uint32_t*)&val->data += 1UL << (EP_POWER_SWITCH); // shift by EID: nth bit set equals EID n exists
      *(uint32_t*)&val->data += 1UL << (EP_POWER_METER);
#if TEMPERATURE_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_TEMPERATURE);
#endif
#if BUTTON_HAS_EID
      *(uint32_t*)&val->data += 1UL << (EP_BUTTON);
#endif
#if HUMIDITY_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_HUMIDITY);
#endif
#if PRESSURE_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_PRESSURE);
#endif
#if ANALOGREAD_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_ANALOGREAD);
#endif
#if METERING_ENERGY
      *(uint32_t*)&val->data += 1UL << (EP_ENERGY_METER_TOTAL);
      *(uint32_t*)&val->data += 1UL << (EP_ENERGY_METER);
#endif
#if SM_UPLOAD_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_SM_CONTROL);
      *(uint32_t*)&val->data += 1UL << (EP_SM_UP_RECEIVER);
      *(uint32_t*)&val->data += 1UL << (EP_SM_UP_ACKNAK);
      *(uint32_t*)&val->data += 1UL << (EP_SM_RESET_ID);
#endif
#if SHUTTER_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_SHUTTER);
#endif
#if HEXAPUSH_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_HEXAPUSH_PRESSED);
      *(uint32_t*)&val->data += 1UL << (EP_HEXAPUSH_CLICKED);
#endif
#if PRESENCE_DETECTOR_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_PRESENCE_DETECTOR);
#endif
#if HEXONOFF_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_HEXONOFF_SET);
      *(uint32_t*)&val->data += 1UL << (EP_HEXONOFF_TOGGLE);
#endif
#if LIGHTSENSOR_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_LIGHTSENSOR);
#endif
#if IR_RECEIVER_ENABLE
      *(uint32_t*)&val->data += 1UL << (EP_IR_RECEIVER);
#endif
      break;
    /* -==================================================================- */

    case EP_POWER_SWITCH:   // Endpoint 1: Hexabus Socket power switch
      val->datatype = HXB_DTYPE_BOOL;
      *(uint8_t*)&val->data = relay_get_state() == 0 ? HXB_TRUE : HXB_FALSE;
      break;
    case EP_POWER_METER:   // Endpoint 2: Hexabus Socket power metering
      val->datatype = HXB_DTYPE_UINT32;
      *(uint32_t*)&val->data = metering_get_power();
      break;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE:   // Endpoint 3: Hexabus temperaure metering
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = temperature_get();
      break;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON:   // Endpoint 4: Pushbutton on the Hexabus-Socket
      val->datatype = HXB_DTYPE_BOOL;
      *(uint8_t*)&val->data = button_get_pushed();
      break;
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY:
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = read_humidity();
      break;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE:
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = read_pressure();
      break;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR:
        val->datatype = HXB_DTYPE_FLOAT;
        *(float*)&val->data = get_lightvalue();
        break;
#endif
#if METERING_ENERGY
    case EP_ENERGY_METER_TOTAL:
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = metering_get_energy_total();
      break;
    case EP_ENERGY_METER:
      val->datatype = HXB_DTYPE_FLOAT;
      *(float*)&val->data = metering_get_energy();
      break;
#endif
#if SM_UPLOAD_ENABLE
    case EP_SM_CONTROL:
      PRINTF("READ on SM_CONTROL EP occurred\n");
      val->datatype = HXB_DTYPE_UINT8;
      if (sm_is_running()) {
        *(uint8_t*)&val->data = STM_STATE_RUNNING;
      } else {
        *(uint8_t*)&val->data = STM_STATE_STOPPED;
      }
      break;
    case EP_SM_UP_RECEIVER:
      PRINTF("READ on SM_UP_RECEIVER EP occurred\n");
      break;
    case EP_SM_UP_ACKNAK:
      PRINTF("READ on SM_UP_ACKNAK EP occurred\n");
      break;
    case EP_SM_RESET_ID:
      PRINTF("READ on SM_RESET_ID EP occurred\n");
      val->datatype = HXB_DTYPE_16BYTES;
      char* b = malloc(HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
      if(b != NULL)
        sm_get_id(b);
      *(char**)&val->data = b;
      break;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER:
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = shutter_get_state();
      break;
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_PRESSED:  //Pressed und released
      val->datatype = HXB_DTYPE_UINT8;
      *(uint8_t*)&val->data = get_buttonstate();
      break;
    case EP_HEXAPUSH_CLICKED: //Clicked
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
    case EP_HEXONOFF_SET:
    case EP_HEXONOFF_TOGGLE:
        val->datatype = HXB_DTYPE_UINT8;
        *(uint8_t*)&val->data = get_outputs();
        break;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD:
        val->datatype = HXB_DTYPE_FLOAT;
        *(float*)&val->data = get_analogvalue();
        break;
#endif
#if IR_RECEIVER_ENABLE
    case EP_IR_RECEIVER:
        val->datatype = HXB_DTYPE_UINT32;
        *(uint32_t*)&val->data = ir_get_last_command();
        break;
#endif
    default:
      val->datatype = HXB_DTYPE_UNDEFINED;
  }
}

