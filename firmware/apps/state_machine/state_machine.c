#include "contiki.h"

#include "hexabus_config.h"
#include "value_broadcast.h"
#include "udp_handler.h"
#include "eeprom_variables.h"
#include "endpoints.h"
#include "state_machine.h"
#include "endpoint_registry.h"
#include "datetime_service.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>      // memcpy

#define LOG_LEVEL STATE_MACHINE_DEBUG
#include "syslog.h"

/*------------------------------------------------------*/
PROCESS_NAME(state_machine_process);
PROCESS(state_machine_process, "State Machine Process");
AUTOSTART_PROCESSES(&state_machine_process);
/*------------------------------------------------------*/

static uint8_t transLength;   // length of transition table
static uint8_t dtTransLength; // length of date/time transition table
static uint8_t curState = 0;  // starting out in state 0
static uint32_t inStateSince; // when did we change into that state
static uint8_t dt_valid;      // internal clock has a valid date/time

static process_event_t sm_handle_input_event;

/**
 * Functions to start/stop the state machine itself.
 */

uint8_t sm_is_running() {
  return (process_is_running(&state_machine_process));
}

void sm_start() {
  if (process_is_running(&state_machine_process)) {
		syslog(LOG_DEBUG, "State machine process is already running - not starting it.");
  } else {
		syslog(LOG_DEBUG, "Starting state machine process.");
    curState = 0;  // always start in state 0
    process_start(&state_machine_process, NULL);
  }
}

void sm_stop() {
  if (!process_is_running(&state_machine_process)) {
		syslog(LOG_DEBUG, "State machine process is already stopped - not stopping it.");
  } else {
		syslog(LOG_DEBUG, "Stopping state machine process.");
    process_exit(&state_machine_process);
  }
}

void sm_restart() {
	syslog(LOG_DEBUG, "Attempting to restart state machine process.");
  sm_stop();
  // init
  sm_start();
	syslog(LOG_DEBUG, "Restart complete");
}

// Evaluates the condition no. condIndex, figure out whether we need to act upon the current hexabus packet
bool eval(uint8_t condIndex, struct hxb_envelope *envelope) {
  struct condition cond;
  // get the condition from eeprom
	syslog(LOG_DEBUG, "Checking condition %d", condIndex);
  if(condIndex == TRUE_COND_INDEX)    // If the condition is set to TRUE
    return true;

	if (!envelope)
		return false;

  sm_get_condition(condIndex, &cond);

	syslog(LOG_DEBUG, "Checking host IP mask.");
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
  if(cond.value.datatype != HXB_DTYPE_DATETIME && cond.value.datatype != HXB_DTYPE_TIMESTAMP && not_anyhost && memcmp(cond.sourceIP, &envelope->src_ip, 16)) { // If not anyhost AND source IP and cond IP differ
		syslog(LOG_DEBUG, "not anyhost AND source IP and cond IP differ");
    return false;
  }
  if(((cond.sourceEID != envelope->eid)) && cond.value.datatype != HXB_DTYPE_DATETIME && cond.value.datatype != HXB_DTYPE_TIMESTAMP) {
		syslog(LOG_DEBUG, "source EID of received packet and condition differ AND not a Date/Time/Timestamp condition.");
    return false;
  }

	syslog(LOG_DEBUG, "IP and EID match / or datetime condition / or anyhost condition");

  // Check datatypes, return false if they don't match -- TIMESTAMP is exempt from that because it's checked alongside the DATETIME conditions
  if(envelope->value.datatype != cond.value.datatype && cond.value.datatype != HXB_DTYPE_TIMESTAMP) {
		syslog(LOG_DEBUG, "datatype mismatch");
    return false;
  }

	syslog(LOG_DEBUG, "Now checking condition");
  // check the actual condition - must be implemented for each datatype individually.
  switch ((enum hxb_datatype) cond.value.datatype)
  {
    case HXB_DTYPE_BOOL:
    case HXB_DTYPE_UINT8:
      if(cond.op == STM_EQ)  return envelope->value.v_u8 == cond.value.v_u8;
      if(cond.op == STM_LEQ) return envelope->value.v_u8 <= cond.value.v_u8;
      if(cond.op == STM_GEQ) return envelope->value.v_u8 >= cond.value.v_u8;
      if(cond.op == STM_LT)  return envelope->value.v_u8 <  cond.value.v_u8;
      if(cond.op == STM_GT)  return envelope->value.v_u8 >  cond.value.v_u8;
      if(cond.op == STM_NEQ) return envelope->value.v_u8 != cond.value.v_u8;
      break;
    case HXB_DTYPE_UINT32:
      if(cond.op == STM_EQ)  return envelope->value.v_u32 == cond.value.v_u32;
      if(cond.op == STM_LEQ) return envelope->value.v_u32 <= cond.value.v_u32;
      if(cond.op == STM_GEQ) return envelope->value.v_u32 >= cond.value.v_u32;
      if(cond.op == STM_LT)  return envelope->value.v_u32 <  cond.value.v_u32;
      if(cond.op == STM_GT)  return envelope->value.v_u32 >  cond.value.v_u32;
      if(cond.op == STM_NEQ) return envelope->value.v_u32 != cond.value.v_u32;
      break;
    case HXB_DTYPE_FLOAT:
      if(cond.op == STM_EQ)  return envelope->value.v_float == cond.value.v_float;
      if(cond.op == STM_LEQ) return envelope->value.v_float <= cond.value.v_float;
      if(cond.op == STM_GEQ) return envelope->value.v_float >= cond.value.v_float;
      if(cond.op == STM_LT)  return envelope->value.v_float <  cond.value.v_float;
      if(cond.op == STM_GT)  return envelope->value.v_float >  cond.value.v_float;
      if(cond.op == STM_NEQ) return envelope->value.v_float != cond.value.v_float;
      break;
    case HXB_DTYPE_DATETIME:
      {
        if(dt_valid)
        {
          struct hxb_datetime val_dt;
          val_dt = envelope->value.v_datetime; // just to make writing this down easier...
          if(cond.op & HXB_SM_HOUR)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.hour >= cond.value.v_u32 : val_dt.hour < cond.value.v_u32;
          if(cond.op & HXB_SM_MINUTE)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.minute >= cond.value.v_u32 : val_dt.minute < cond.value.v_u32;
          if(cond.op & HXB_SM_SECOND)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.second >= cond.value.v_u32 : val_dt.second < cond.value.v_u32;
          if(cond.op & HXB_SM_DAY)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.day >= cond.value.v_u32 : val_dt.day < cond.value.v_u32;
          if(cond.op & HXB_SM_MONTH)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.month >= cond.value.v_u32 : val_dt.month < cond.value.v_u32;
          if(cond.op & HXB_SM_YEAR)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.year >= cond.value.v_u32 : val_dt.year < cond.value.v_u32;
          if(cond.op & HXB_SM_WEEKDAY)
            return (cond.op & HXB_SM_DATETIME_OP_GEQ) ? val_dt.weekday >= cond.value.v_u32 : val_dt.weekday < cond.value.v_u32;
        }
        break;
      }
    case HXB_DTYPE_TIMESTAMP:
      if(cond.op == HXB_SM_TIMESTAMP_OP) // in-state-since
      {
				syslog(LOG_DEBUG, "Checking in-state-since Condition! Have been in this state for %lu sec.", getTimestamp() - inStateSince);
				syslog(LOG_DEBUG, "getTimestamp(): %lu - inStateSince: %lu >= cond.value.data: %lu", getTimestamp(), inStateSince, cond.value.v_u32);
        return getTimestamp() - inStateSince >= cond.value.v_u32;
      }
      break;

		case HXB_DTYPE_128STRING:
		case HXB_DTYPE_16BYTES:
		case HXB_DTYPE_65BYTES:
		case HXB_DTYPE_UNDEFINED:
    default:
			syslog(LOG_DEBUG, "Datatype not implemented in state machine (yet).");
      return false;
  }

	return false;
}

void check_datetime_transitions()
{
	// TODO maybe we should check timestamps separately, because as of now, they still need some special cases in the 'eval' function
	struct hxb_envelope dtenvelope;
	dtenvelope.value.datatype = HXB_DTYPE_DATETIME;
	dt_valid = 1 + getDatetime(&dtenvelope.value.v_datetime);   // getDatetime returns -1 if it fails, 0 if it succeeds, so we have to "1 +" here
	struct transition t;

	struct hxb_envelope eid_env;
	eid_env.src_port = 0;
	uip_ipaddr(&eid_env.src_ip, 0, 0, 0, 1);

	for (uint8_t i = 0; i < dtTransLength; i++) {
		//eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_DATETIME_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
		sm_get_transition(true, i, &t);

		syslog(LOG_DEBUG, "checkDT - curState: %d -- fromState: %d", curState, t.fromState);
		if ((t.fromState == curState) && (eval(t.cond, &dtenvelope))) {
			// Matching transition found. Check, if an action should be performed.
			if (t.eid != 0 || t.value.datatype != 0) {
				// Try executing the command
				syslog(LOG_DEBUG, "Writing to endpoint %ld", t.eid);
				eid_env.value = t.value;
				eid_env.eid = t.eid;
				if (endpoint_write(t.eid, &eid_env) == 0) {
					inStateSince = getTimestamp();
					curState = t.goodState;
					syslog(LOG_DEBUG, "Everything is fine");
					break;
				} else {
					inStateSince = getTimestamp();
					curState = t.badState;
					syslog(LOG_DEBUG, "Something bad happened");
					break;
				}
			} else {
				inStateSince = getTimestamp();
				curState = t.goodState;
				syslog(LOG_DEBUG, "No action performed.");
			}
		}
	}
}

void check_value_transitions(void* data)
{
	struct hxb_envelope* env = (struct hxb_envelope*) data;
	struct transition t;

	struct hxb_envelope eid_env;
	eid_env.src_port = 0;
	uip_ipaddr(&eid_env.src_ip, 0, 0, 0, 1);

	for (uint8_t i = 0; i < transLength; i++) {
		//eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
		sm_get_transition(false, i, &t);
		// get next transition to check from eeprom
		if ((t.fromState == curState) && (eval(t.cond, env))) {
			// Match found
			if (t.eid != 0 || t.value.datatype != 0) {
				eid_env.eid = t.eid;
				eid_env.value = t.value;
				syslog(LOG_DEBUG, "Writing to endpoint %ld", t.eid);
				if (endpoint_write(t.eid, &eid_env) == 0) {
					inStateSince = getTimestamp();
					curState = t.goodState;
					syslog(LOG_DEBUG, "Everything is fine");
					break;
				} else { // Something went wrong
					inStateSince = getTimestamp();
					curState = t.badState;
					syslog(LOG_DEBUG, "Something bad happened");
					break;
				}
			} else {
				inStateSince = getTimestamp();
				curState = t.goodState;
				syslog(LOG_DEBUG, "No action performed");
			}
		}
	}
}

void sm_handle_input(struct hxb_envelope* data)
{
	process_post_synch(&state_machine_process, sm_handle_input_event, data);
}

static void print_transition_table(bool dttrans)
{
	int length = sm_get_number_of_transitions(dttrans);

	if (dttrans) {
		syslog(LOG_DEBUG, "[date/time table size: %d]:", transLength);
	} else {
		syslog(LOG_DEBUG, "[State machine table size: %d]:", transLength);
	}
	syslog(LOG_DEBUG, "From | Cond |      EID | Good | Bad");
	for (int k = 0; k < length; k++) {
		struct transition tt;
		sm_get_transition(dttrans, k, &tt);
		syslog(LOG_DEBUG, "%4d | %4d | %8ld | %4d | %3d", tt.fromState, tt.cond, tt.eid, tt.goodState, tt.badState);
	}
	syslog(LOG_DEBUG, "-----------------------------------");
}

PROCESS_THREAD(state_machine_process, ev, data)
{
  PROCESS_BEGIN();

	sm_handle_input_event = process_alloc_event();

  // read state machine table length from eeprom
  transLength = sm_get_number_of_transitions(false);   //eeprom_read_byte((void*)EE_STATEMACHINE_TRANSITIONS);
  dtTransLength = sm_get_number_of_transitions(true);  //eeprom_read_byte((void*)EE_STATEMACHINE_DATETIME_TRANSITIONS);

#if STATE_MACHINE_DEBUG
	print_transition_table(false);
	print_transition_table(true);
#endif // STATE_MACHINE_DEBUG

  // initialize timer
  static struct etimer check_timer;
  etimer_set(&check_timer, CLOCK_SECOND * 5); // TODO do we want this configurable?

  while(sm_is_running())
  {
    PROCESS_WAIT_EVENT();
		if (!sm_is_running())
			break;

		if (ev == udp_handler_event && *(udp_handler_event_t*) data == UDP_HANDLER_UP) {
			// the udp handler has transitioned to "ready" for some reason, most likely a cold reboot
			// since anything that can bring the udp handler down will also affect the state machine, reset it
			// special case: ID 0 is reserved for state machines that should not be automatically reset
			if (!sm_id_is_zero()) {
				syslog(LOG_NOTICE, "UDP handler reset, broadcasting Reset-ID");
				broadcast_value(EP_SM_RESET_ID);
				curState = 0;
				// also, unconditionally evaluate value transitions.
				// with no value, only TRUE transitions can fire, including the often used "initialization" TRUE transition
				check_value_transitions(NULL);
			}
		} else {
			syslog(LOG_DEBUG, "Received event, current state: %d", curState);
			if (ev == PROCESS_EVENT_TIMER) {
				check_datetime_transitions();
				etimer_reset(&check_timer);
			} else if (ev == sm_handle_input_event) {
				if(!sm_id_is_zero()) { // if our state machine ID is not 0 (0 means "no auto reset".
					// check if it's a Reset-ID (this means another state machine on the network was just unexpectedly reset)
					struct hxb_envelope* envelope = (struct hxb_envelope*)data;
					syslog(LOG_DEBUG, "SM_ID is not zero.");
					if(envelope->eid == EP_SM_RESET_ID && envelope->value.datatype == HXB_DTYPE_16BYTES) {
						uint32_t localhost[] = { 0, 0, 0, 0x01000000 }; // that's ::1 in IPv6 notation
						syslog(LOG_NOTICE, "Received Reset-ID from " LOG_6ADDR_FMT, LOG_6ADDR_VAL(envelope->src_ip));
						if(memcmp(&envelope->src_ip, &localhost , 16)) { // Ignore resets from localhost (the reset ID from localhost was sent BECAUSE we just reset)
							char* received_sm_id = envelope->value.v_binary;
							char own_sm_id[16];
							sm_get_id(own_sm_id);
							if(!memcmp(received_sm_id, own_sm_id, 16))
								sm_restart();
						}
					}
				}

				// check the values + endpoints.
				check_value_transitions(data);

				syslog(LOG_DEBUG, "Now in state: %d", curState);
			}
    }
  }

  PROCESS_END();
}

