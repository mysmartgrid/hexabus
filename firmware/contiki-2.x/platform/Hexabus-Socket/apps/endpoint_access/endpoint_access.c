// Abstraction layer - transforms generic endpoint access to specific hardware function calls
// ============================================================================

#include "hexabus_config.h"
#include "endpoint_access.h"
#include "temperature.h"
#include "button_handlers.h"
#include "analogread.h"
#include "humidity.h"
#include "relay.h"
#include "button_handlers.h"
#include "hexapush.h"
#include "state_machine.h"
#include "pressure.h"
#include "ir_receiver.h"
#include "metering.h"
#include "endpoints.h"
#include "net/uip.h"
#include "packet_builder.h"
#include "udp_handler.h"
#include <stdbool.h>
#include "eeprom_variables.h"
#include <string.h>
#include <stdlib.h>

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
      eeprom_read_block(buffer, (void*)(EE_DOMAIN_NAME), EE_DOMAIN_NAME_SIZE);
      buffer[EE_DOMAIN_NAME_SIZE] = '\0';
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
				if (value->v_bool == HXB_TRUE)
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
        if(value->v_u8 == STM_STATE_STOPPED ) {
          sm_stop();
        } else if(value->v_u8 == STM_STATE_RUNNING) {
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
        char* payload = value->v_string;
        uint8_t chunk_id = (uint8_t)payload[0];
        if (sm_write_chunk(chunk_id, payload+1) == true) {
          // send ACK to SM_SENDER
          PRINTF("SENDING ACK");
          val.v_bool = HXB_TRUE;
          struct hxb_packet_int8 packet = make_value_packet_int8(EP_SM_UP_ACKNAK, &val);
          send_packet((char*)&packet, sizeof(packet));
          return 0;
        } else {
          // send NAK to SM_SENDER
          PRINTF("SENDING NAK");
          val.v_bool = HXB_FALSE;
          struct hxb_packet_int8 packet = make_value_packet_int8(EP_SM_UP_ACKNAK, &val);
          send_packet((char*)&packet, sizeof(packet));
          return HXB_ERR_INVALID_VALUE;
        }
      } else {
        // send NAK to SM_SENDER
        PRINTF("SENDING ERROR DATATYPE");
        val.v_bool = HXB_FALSE;
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
          shutter_toggle(value->v_u8);
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
        if(value->v_u8 == 1)
        {
            global_presence_detected();
        } else if(value->v_u8 == 0) {
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
            set_outputs(value->v_u8);
        } else {
            return HXB_ERR_DATATYPE;
        }
        break;
    case EP_HEXONOFF_TOGGLE:
        if(value->datatype == HXB_DTYPE_UINT8) {
            toggle_outputs(value->v_u8);
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
      val->v_u32 = 1UL;       // start out with last bit set: EID0 always exists
      // endpoints 1 and 2 are not optional
      val->v_u32 += 1UL << (EP_POWER_SWITCH); // shift by EID: nth bit set equals EID n exists
      val->v_u32 += 1UL << (EP_POWER_METER);
#if TEMPERATURE_ENABLE
      val->v_u32 += 1UL << (EP_TEMPERATURE);
#endif
#if BUTTON_HAS_EID
      val->v_u32 += 1UL << (EP_BUTTON);
#endif
#if HUMIDITY_ENABLE
      val->v_u32 += 1UL << (EP_HUMIDITY);
#endif
#if PRESSURE_ENABLE
      val->v_u32 += 1UL << (EP_PRESSURE);
#endif
#if ANALOGREAD_ENABLE
      val->v_u32 += 1UL << (EP_ANALOGREAD);
#endif
#if METERING_ENERGY
      val->v_u32 += 1UL << (EP_ENERGY_METER_TOTAL);
      val->v_u32 += 1UL << (EP_ENERGY_METER);
#endif
#if SM_UPLOAD_ENABLE
      val->v_u32 += 1UL << (EP_SM_CONTROL);
      val->v_u32 += 1UL << (EP_SM_UP_RECEIVER);
      val->v_u32 += 1UL << (EP_SM_UP_ACKNAK);
      val->v_u32 += 1UL << (EP_SM_RESET_ID);
#endif
#if SHUTTER_ENABLE
      val->v_u32 += 1UL << (EP_SHUTTER);
#endif
#if HEXAPUSH_ENABLE
      val->v_u32 += 1UL << (EP_HEXAPUSH_PRESSED);
      val->v_u32 += 1UL << (EP_HEXAPUSH_CLICKED);
#endif
#if PRESENCE_DETECTOR_ENABLE
      val->v_u32 += 1UL << (EP_PRESENCE_DETECTOR);
#endif
#if HEXONOFF_ENABLE
      val->v_u32 += 1UL << (EP_HEXONOFF_SET);
      val->v_u32 += 1UL << (EP_HEXONOFF_TOGGLE);
#endif
#if LIGHTSENSOR_ENABLE
      val->v_u32 += 1UL << (EP_LIGHTSENSOR);
#endif
#if IR_RECEIVER_ENABLE
      val->v_u32 += 1UL << (EP_IR_RECEIVER);
#endif
      break;
    /* -==================================================================- */

    case EP_POWER_SWITCH:   // Endpoint 1: Hexabus Socket power switch
      val->datatype = HXB_DTYPE_BOOL;
      val->v_bool = relay_get_state() == 0 ? HXB_TRUE : HXB_FALSE;
      break;
    case EP_POWER_METER:   // Endpoint 2: Hexabus Socket power metering
      val->datatype = HXB_DTYPE_UINT32;
      val->v_u32 = metering_get_power();
      break;
#if TEMPERATURE_ENABLE
    case EP_TEMPERATURE:   // Endpoint 3: Hexabus temperaure metering
      val->datatype = HXB_DTYPE_FLOAT;
      val->v_float = temperature_get();
      break;
#endif
#if BUTTON_HAS_EID
    case EP_BUTTON:   // Endpoint 4: Pushbutton on the Hexabus-Socket
      val->datatype = HXB_DTYPE_BOOL;
      val->v_bool = button_pushed;
      break;
#endif
#if HUMIDITY_ENABLE
    case EP_HUMIDITY:
      val->datatype = HXB_DTYPE_FLOAT;
      val->v_float = read_humidity();
      break;
#endif
#if PRESSURE_ENABLE
    case EP_PRESSURE:
      val->datatype = HXB_DTYPE_FLOAT;
      val->v_float = read_pressure();
      break;
#endif
#if LIGHTSENSOR_ENABLE
    case EP_LIGHTSENSOR:
        val->datatype = HXB_DTYPE_FLOAT;
        val->v_float = get_lightvalue();
        break;
#endif
#if METERING_ENERGY
    case EP_ENERGY_METER_TOTAL:
      val->datatype = HXB_DTYPE_FLOAT;
      val->v_float = metering_get_energy_total();
      break;
    case EP_ENERGY_METER:
      val->datatype = HXB_DTYPE_FLOAT;
      val->v_float = metering_get_energy();
      break;
#endif
#if SM_UPLOAD_ENABLE
    case EP_SM_CONTROL:
      PRINTF("READ on SM_CONTROL EP occurred\n");
      val->datatype = HXB_DTYPE_UINT8;
      if (sm_is_running()) {
        val->v_u8 = STM_STATE_RUNNING;
      } else {
        val->v_u8 = STM_STATE_STOPPED;
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
      val->v_binary = b;
      break;
#endif
#if SHUTTER_ENABLE
    case EP_SHUTTER:
      val->datatype = HXB_DTYPE_UINT8;
      val->v_u8 = shutter_get_state();
      break;
#endif
#if HEXAPUSH_ENABLE
    case EP_HEXAPUSH_PRESSED:  //Pressed und released
      val->datatype = HXB_DTYPE_UINT8;
      val->v_u8 = hexapush_get_pressed();
      break;
    case EP_HEXAPUSH_CLICKED: //Clicked
      val->datatype = HXB_DTYPE_UINT8;
      val->v_u8 = hexapush_get_clicked();
      break;
#endif
#if PRESENCE_DETECTOR_ENABLE
    case EP_PRESENCE_DETECTOR:
      val->datatype = HXB_DTYPE_UINT8;
      val->v_u8 = is_presence();
      break;
#endif
#if HEXONOFF_ENABLE
    case EP_HEXONOFF_SET:
    case EP_HEXONOFF_TOGGLE:
        val->datatype = HXB_DTYPE_UINT8;
        val->v_u8 = get_outputs();
        break;
#endif
#if ANALOGREAD_ENABLE
    case EP_ANALOGREAD:
        val->datatype = HXB_DTYPE_FLOAT;
        val->v_float = get_analogvalue();
        break;
#endif
#if IR_RECEIVER_ENABLE
    case EP_IR_RECEIVER:
        val->datatype = HXB_DTYPE_UINT32;
        val->v_u32 = ir_get_last_command();
        break;
#endif
    default:
      val->datatype = HXB_DTYPE_UNDEFINED;
  }
}

