#include "contiki.h"

#include "hexabus_config.h"
#include "value_broadcast.h"
#include "eeprom_variables.h"
#include "endpoints.h"
#include "../../../../../../shared/hexabus_packet.h"
#include "state_machine.h"
#include "endpoint_access.h"
#include "datetime_service.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>      // memcpy

#if STATE_MACHINE_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/*------------------------------------------------------*/
PROCESS(state_machine_process, "State Machine Process");
AUTOSTART_PROCESSES(&state_machine_process);
/*------------------------------------------------------*/

static uint8_t transLength;   // length of transition table
static uint8_t dtTransLength; // length of date/time transition table
static uint8_t curState = 0;  // starting out in state 0
static uint32_t inStateSince; // when did we change into that state
static uint8_t dt_valid;      // internal clock has a valid date/time

/**
 * Functions to start/stop the state machine itself.
 */

uint8_t sm_is_running() {
  return (process_is_running(&state_machine_process));
}

void sm_start() {
  if (process_is_running(&state_machine_process)) {
    PRINTF("State machine process is already running - not starting it.\n");
  } else {
    PRINTF("Starting state machine process.\n");
    curState = 0;  // always start in state 0
    process_start(&state_machine_process, NULL);
  }
}

void sm_stop() {
  if (!process_is_running(&state_machine_process)) {
    PRINTF("State machine process is already stopped - not stopping it.\n");
  } else {
    PRINTF("Stopping state machine process.\n");
    process_exit(&state_machine_process);
  }
}

void sm_restart() {
  PRINTF("Attempting to restart state machine process.\n");
  sm_stop();
  // init
  sm_start();
  PRINTF("Restart complete");
}

// Evaluates the condition no. condIndex, figure out whether we need to act upon the current hexabus packet
bool eval(uint8_t condIndex, struct hxb_envelope *envelope) {
  struct condition cond;
  // get the condition from eeprom
  PRINTF("Checking condition %d\r\n", condIndex);
  if(condIndex == TRUE_COND_INDEX)    // If the condition is set to TRUE
    return true;

  sm_get_condition(condIndex, &cond);

  PRINTF("Checking host IP mask.\r\n");
  // check for ANYHOST condition (something other than :: (all zeroes))
  // not_anyhost is == 0 if it's an anyhost condition
  uint8_t not_anyhost = 16;
  while(not_anyhost)
  {
    if(cond.sourceIP[not_anyhost - 1])
      break;  // this breaks once it finds something other than zero, leaving not_anyhost > 0. If all the address bytes are zero, not_anyhost counts all the way down to 0.
    not_anyhost--;
  }

  // Check IPs and EID:
  // If IP is all zero -> return without any action.
  //ignore IP and EID for Datetime-Conditions and Timestamp-Conditions
  if(cond.value.datatype != HXB_DTYPE_DATETIME && cond.value.datatype != HXB_DTYPE_TIMESTAMP && not_anyhost && memcmp(cond.sourceIP, envelope->source, 16)) { // If not anyhost AND source IP and cond IP differ
    PRINTF("not anyhost AND source IP and cond IP differ\r\n");
    return false;
  }
  if(((cond.sourceEID != envelope->eid)) && cond.value.datatype != HXB_DTYPE_DATETIME && cond.value.datatype != HXB_DTYPE_TIMESTAMP) {
    PRINTF("source EID of received packet and condition differ AND not a Date/Time/Timestamp condition.\r\n");
    return false;
  }

  PRINTF("IP and EID match / or datetime condition / or anyhost condition\r\n");

  // Check datatypes, return false if they don't match -- TIMESTAMP is exempt from that because it's checked alongside the DATETIME conditions
  if(envelope->value.datatype != cond.value.datatype && cond.value.datatype != HXB_DTYPE_TIMESTAMP) {
    PRINTF("datatype mismatch");
    return false;
  }

  PRINTF("Now checking condition\r\n");
  // check the actual condition - must be implemented for each datatype individually.
  switch(cond.value.datatype)
  {
    case HXB_DTYPE_BOOL:
    case HXB_DTYPE_UINT8:
      if(cond.op == STM_EQ)  return *(uint8_t*)&envelope->value.data == cond.value.data[0];
      if(cond.op == STM_LEQ) return *(uint8_t*)&envelope->value.data <= cond.value.data[0];
      if(cond.op == STM_GEQ) return *(uint8_t*)&envelope->value.data >= cond.value.data[0];
      if(cond.op == STM_LT)  return *(uint8_t*)&envelope->value.data <  cond.value.data[0];
      if(cond.op == STM_GT)  return *(uint8_t*)&envelope->value.data >  cond.value.data[0];
      if(cond.op == STM_NEQ) return *(uint8_t*)&envelope->value.data != cond.value.data[0];
      break;
    case HXB_DTYPE_UINT32:
      if(cond.op == STM_EQ)  return *(uint32_t*)&envelope->value.data == *(uint32_t*)&cond.value.data;
      if(cond.op == STM_LEQ) return *(uint32_t*)&envelope->value.data <= *(uint32_t*)&cond.value.data;
      if(cond.op == STM_GEQ) return *(uint32_t*)&envelope->value.data >= *(uint32_t*)&cond.value.data;
      if(cond.op == STM_LT)  return *(uint32_t*)&envelope->value.data <  *(uint32_t*)&cond.value.data;
      if(cond.op == STM_GT)  return *(uint32_t*)&envelope->value.data >  *(uint32_t*)&cond.value.data;
      if(cond.op == STM_NEQ) return *(uint32_t*)&envelope->value.data != *(uint32_t*)&cond.value.data;
      break;
    case HXB_DTYPE_FLOAT:
      if(cond.op == STM_EQ)  return *(float*)&envelope->value.data == *(float*)&cond.value.data;
      if(cond.op == STM_LEQ) return *(float*)&envelope->value.data <= *(float*)&cond.value.data;
      if(cond.op == STM_GEQ) return *(float*)&envelope->value.data >= *(float*)&cond.value.data;
      if(cond.op == STM_LT)  return *(float*)&envelope->value.data <  *(float*)&cond.value.data;
      if(cond.op == STM_GT)  return *(float*)&envelope->value.data >  *(float*)&cond.value.data;
      if(cond.op == STM_NEQ) return *(float*)&envelope->value.data != *(float*)&cond.value.data;
      break;
    case HXB_DTYPE_DATETIME:
      {
        if(dt_valid)
        {
          struct datetime val_dt;
          val_dt = *(struct datetime*)&envelope->value.data; // just to make writing this down easier...
          if(cond.op & HXB_SM_HOUR)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.hour >= *(uint8_t*)&(cond.value.data) : val_dt.hour < *(uint8_t*)&(cond.value.data);
          if(cond.op & HXB_SM_MINUTE)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.minute >= *(uint8_t*)&(cond.value.data) : val_dt.minute < *(uint8_t*)&(cond.value.data);
          if(cond.op & HXB_SM_SECOND)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.second >= *(uint8_t*)&(cond.value.data) : val_dt.second < *(uint8_t*)&(cond.value.data);
          if(cond.op & HXB_SM_DAY)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.day >= *(uint8_t*)&(cond.value.data) : val_dt.day < *(uint8_t*)&(cond.value.data);
          if(cond.op & HXB_SM_MONTH)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.month >= *(uint8_t*)&(cond.value.data) : val_dt.month < *(uint8_t*)&(cond.value.data);
          if(cond.op & HXB_SM_YEAR)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.year >= *(uint16_t*)&(cond.value.data) : val_dt.year < *(uint16_t*)&(cond.value.data);
          if(cond.op & HXB_SM_WEEKDAY)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.weekday >= *(uint8_t*)&(cond.value.data) : val_dt.weekday < *(uint8_t*)&(cond.value.data);
        }
        break;
      }
    case HXB_DTYPE_TIMESTAMP:
      if(cond.op == HXB_SM_TIMESTAMP_OP) // in-state-since
      {
        PRINTF("Checking in-state-since Condition! Have been in this state for %lu sec.\r\n", getTimestamp() - inStateSince);
        PRINTF("getTimestamp(): %lu - inStateSince: %lu >= cond.value.data: %lu\r\n", getTimestamp(), inStateSince, *(uint32_t*)&cond.value.data);
        return getTimestamp() - inStateSince >= *(uint32_t*)&cond.value.data;
      }
      break;
    default:
      PRINTF("Datatype not implemented in state machine (yet).\r\n");
      return false;
  }
}

void check_datetime_transitions()
{
  // TODO maybe we should check timestamps separately, because as of now, they still need some special cases in the 'eval' function
  struct hxb_envelope dtenvelope;
  dtenvelope.value.datatype = HXB_DTYPE_DATETIME;
  dt_valid = 1 + getDatetime(&dtenvelope.value.data);   // getDatetime returns -1 if it fails, 0 if it succeeds, so we have to "1 +" here
  struct transition* t = malloc(sizeof(struct transition));

  uint8_t i;
  for(i = 0; i < dtTransLength; i++)
  {
    //eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_DATETIME_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
    sm_get_transition(true, i, t);
    PRINTF("checkDT - curState: %d -- fromState: %d\r\n", curState, t->fromState);
    if((t->fromState == curState) && (eval(t->cond, &dtenvelope)))
    {
      // Matching transition found. Check, if an action should be performed.
      if(t->eid != 0 || t->value.datatype != 0)
      {
        // Try executing the command
        PRINTF("state_machine: Writing to endpoint %d\r\n", t->eid);
        if(endpoint_write(t->eid, &(t->value)) == 0)
        {
          inStateSince = getTimestamp();
          curState = t->goodState;
          PRINTF("state_machine: Everything is fine \r\n");
          break;
        } else {
          inStateSince = getTimestamp();
          curState = t->badState;
          PRINTF("state_machine: Something bad happened \r\n");
          break;
        }
      } else {
        inStateSince = getTimestamp();
        curState = t->goodState;
        PRINTF("state_machine: No action performed. \r\n");
      }
    }
  }
  free(t);
}

void check_value_transitions(void* data)
{
  uint8_t i; // for the for-loops
  struct transition* t = malloc(sizeof(struct transition));

  struct hxb_envelope* envelope = (struct hxb_envelope*)data;
  for(i = 0; i < transLength; i++)
  {
    //eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
    sm_get_transition(false, i, t);
    // get next transition to check from eeprom
    if((t->fromState == curState) && (eval(t->cond, envelope)))
    {
      // Match found
      if(t->eid != 0 || t->value.datatype != 0)
      {
        PRINTF("state_machine: Writing to endpoint %d \r\n", t->eid);
        if(endpoint_write(t->eid, &(t->value)) == 0)
        {
          inStateSince = getTimestamp();
          curState = t->goodState;
          PRINTF("state_machine: Everything is fine \r\n");
          break;
        } else {                          // Something went wrong
          inStateSince = getTimestamp();
          curState = t->badState;
          PRINTF("state_machine: Something bad happened \r\n");
          break;
        }
      } else {
        inStateSince = getTimestamp();
        curState = t->goodState;
        PRINTF("state_machine: No action performed. \r\n");
      }
    }
  }
  free(t);
}

PROCESS_THREAD(state_machine_process, ev, data)
{
  PROCESS_BEGIN();

  // read state machine table length from eeprom
  transLength = sm_get_number_of_transitions(false);   //eeprom_read_byte((void*)EE_STATEMACHINE_TRANSITIONS);
  dtTransLength = sm_get_number_of_transitions(true);  //eeprom_read_byte((void*)EE_STATEMACHINE_DATETIME_TRANSITIONS);

#if STATE_MACHINE_DEBUG
  // output tables so we see if reading it works
  struct transition* tt = malloc(sizeof(struct transition));
  if(tt != NULL)
  {
    int k = 0;
    PRINTF("[State machine table size: %d]:\r\nFrom | Cond | EID | Good | Bad\r\n", transLength);
    for(k = 0;k < transLength;k++)
    {
      //eeprom_read_block(tt, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (k * sizeof(struct transition))), sizeof(struct transition));
      sm_get_transition(false, k, tt);
      PRINTF(" %d | %d | %d | %d | %d \r\n", tt->fromState, tt->cond, tt->eid, tt->goodState, tt->badState);
    }
    PRINTF("[date/time table size: %d]:\r\nFrom | Cond | EID | Good | Bad\r\n", dtTransLength);
    for(k = 0;k < dtTransLength;k++)
    {
      //eeprom_read_block(tt, (void*)(1 + EE_STATEMACHINE_DATETIME_TRANSITIONS + (k * sizeof(struct transition))), sizeof(struct transition));
      sm_get_transition(true, k, tt);
      PRINTF(" %d | %d | %d | %d | %d \r\n", tt->fromState, tt->cond, tt->eid, tt->goodState, tt->badState);
    }
  } else {
    PRINTF("malloc failed!\r\n");
  }
  free(tt);
#endif // STATE_MACHINE_DEBUG

  // initialize timer
  static struct etimer check_timer;
  etimer_set(&check_timer, CLOCK_SECOND * 5); // TODO do we want this configurable?

  // If we've been cold-rebooted (power outage?), send out a broadcast telling everyone so.
  static struct ctimer sm_bootup_timer; // we need some time so the network can initialize
  static uint32_t ep_sm_reset_id = EP_SM_RESET_ID;
  // assume it's a reboot if the timestamp (time since bootup) is less than 1 (second)
  if(getTimestamp() < 1 && !sm_id_is_zero()) { // only broadcast if ID is not 0 -- 0 is reserved for "no-auto-reset".
    PRINTF("State machine: Assuming reboot, broadcasting Reset-ID\n");
    ctimer_set(&sm_bootup_timer, 3 * CLOCK_SECOND, broadcast_value_ptr, (void*)&ep_sm_reset_id); // TODO do we want the timeout configurable?
  }

  while(sm_is_running())
  {
    PROCESS_WAIT_EVENT();
    if(sm_is_running()) {
      PRINTF("State machine: Received event\r\n");
      PRINTF("state machine: Current state: %d\r\n", curState);
      if(ev == PROCESS_EVENT_TIMER)
      {
        check_datetime_transitions();
        etimer_reset(&check_timer);
      }
      if(ev == sm_data_received_event)
      {
        if(!sm_id_is_zero()) { // if our state machine ID is not 0 (0 means "no auto reset".
          // check if it's a Reset-ID (this means another state machine on the network was just unexpectedly reset)
          struct hxb_envelope* envelope = (struct hxb_envelope*)data;
          PRINTF("SM_ID is not zero.");
          if(envelope->eid == EP_SM_RESET_ID && envelope->value.datatype == HXB_DTYPE_16BYTES) {
            uint32_t localhost[] = { 0, 0, 0, 0x01000000 }; // that's ::1 in IPv6 notation
            PRINTF("State machine: Received Reset-ID from ");
            PRINT6ADDR(envelope->source);
            PRINTF("\n");
            if(memcmp(&(envelope->source), &localhost , 16)) { // Ignore resets from localhost (the reset ID from localhost was sent BECAUSE we just reset)
              uint8_t* received_sm_id = *(uint8_t**)&envelope->value.data;
              uint8_t own_sm_id[16];
              sm_get_id(own_sm_id);
              if(!memcmp(received_sm_id, own_sm_id, 16))
                sm_restart();
            }
          }
        }

        // check the values + endpoints.
        check_value_transitions(data);
        free(data);

        PRINTF("state machine: Now in state: %d\r\n", curState);
      }
    } else {
      PRINTF("State machine is not running - discarding event.\r\n");
    }
  }

  PROCESS_END();
}

