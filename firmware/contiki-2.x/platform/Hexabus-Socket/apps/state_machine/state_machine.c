#include "contiki.h"

#include "hexabus_config.h"
#include "eeprom_variables.h"
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
#define TRUE_COND_INDEX 255

/*------------------------------------------------------*/
PROCESS(state_machine_process, "State Machine Process");
AUTOSTART_PROCESSES(&state_machine_process);
/*------------------------------------------------------*/

static uint8_t transLength;   // length of transition table
static uint8_t dtTransLength; // length of date/time transition table
static uint8_t curState = 0;  // starting out in state 0
static uint32_t inStateSince; // when die we change into that state
static uint8_t dt_valid;      // internal clock has a valid date/time

bool eval(uint8_t condIndex, struct hxb_envelope *envelope) {
  struct condition cond;
  // get the condition from eeprom
  PRINTF("Checking condition %d\r\n", condIndex);
  if(condIndex == TRUE_COND_INDEX)		// If the condition is set to TRUE
		return true;
	
	eeprom_read_block(&cond, (void*)(EE_STATEMACHINE_CONDITIONS + (condIndex * sizeof(struct condition))), sizeof(struct condition));

  // check if host is set (something other than :: (all zeroes)) -- if source host is ::, don't care for the source IP (anyhost condition)
  uint8_t hostset = 16;
  while(hostset)
  {
    if(cond.sourceIP[hostset - 1])
      break;  // this breaks once it finds something other than zero, leaving hostset > 0. If all the address bytes are zero, hostset counts all the way down to 0.
    hostset--;
  }

  // Check IPs and EID; ignore IP and EID for Datetime-Conditions and Timestamp-Conditions
  if(hostset && (memcmp(cond.sourceIP, envelope->source, 16) || (cond.sourceEID != envelope->eid)) && cond.value.datatype != HXB_DTYPE_DATETIME && cond.value.datatype != HXB_DTYPE_TIMESTAMP)
    return false;

  PRINTF("IP and EID match / or datetime condition / or anyhost condition\r\n");

  // Check datatypes, return false if they don't match -- TIMESTAMP is exempt from that because it's checked alongside the DATETIME conditions
  if(envelope->value.datatype != cond.value.datatype && cond.value.datatype != HXB_DTYPE_TIMESTAMP) {
    return false;
  }

  PRINTF("Now checking condition\r\n");
  // check the actual condition
  switch(cond.value.datatype)
  {
    case HXB_DTYPE_BOOL:
    case HXB_DTYPE_UINT8:
      if(cond.op == STM_EQ)  return *(uint8_t*)&envelope->value.data == *(uint8_t*)&cond.value.data;
      if(cond.op == STM_LEQ) return *(uint8_t*)&envelope->value.data <= *(uint8_t*)&cond.value.data;
      if(cond.op == STM_GEQ) return *(uint8_t*)&envelope->value.data >= *(uint8_t*)&cond.value.data;
      if(cond.op == STM_LT)  return *(uint8_t*)&envelope->value.data <  *(uint8_t*)&cond.value.data;
      if(cond.op == STM_GT)  return *(uint8_t*)&envelope->value.data >  *(uint8_t*)&cond.value.data;
      if(cond.op == STM_NEQ) return *(uint8_t*)&envelope->value.data != *(uint8_t*)&cond.value.data;
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
        struct datetime cond_dt;
        val_dt = *(struct datetime*)&envelope->value.data; // just to make writing this down easier...
        cond_dt = *(struct datetime*)&cond.value.data;
        if(cond.op & 0x01) // hour
          return (cond.op & 0x80) ? val_dt.hour >= cond_dt.hour : val_dt.hour < cond_dt.hour;
        if(cond.op & 0x02) // minute
          return (cond.op & 0x80) ? val_dt.minute >= cond_dt.minute : val_dt.minute < cond_dt.minute;
        if(cond.op & 0x04) // second
          return (cond.op & 0x80) ? val_dt.second >= cond_dt.second : val_dt.second < cond_dt.second;
        if(cond.op & 0x08) // day
          return (cond.op & 0x80) ? val_dt.day >= cond_dt.day : val_dt.day < cond_dt.day;
        if(cond.op & 0x10) // month
          return (cond.op & 0x80) ? val_dt.month >= cond_dt.month : val_dt.month < cond_dt.month;
        if(cond.op & 0x20) // year
          return (cond.op & 0x80) ? val_dt.year >= cond_dt.year : val_dt.year < cond_dt.year;
        if(cond.op & 0x40) // weekday
          return (cond.op & 0x80) ? val_dt.weekday >= cond_dt.weekday : val_dt.weekday < cond_dt.weekday;
      }
      break;
    }
    case HXB_DTYPE_TIMESTAMP:
      if(cond.op == 0x80) // in-state-since
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
  dt_valid = 1 - getDatetime(&dtenvelope.value.data);   // getDatetime returns 1 if it fails, 0 if it succeeds, so we have to "1 -" here
  struct transition* t = malloc(sizeof(struct transition));

  uint8_t i;
  for(i = 0; i < dtTransLength; i++)
  {
    eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_DATETIME_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
    PRINTF("checkDT - curState: %d -- fromState: %d", curState, t->fromState);
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
  for(i = 0;i < transLength;i++)
  {
		eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
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
				PRINTF("state_machine: No action performed. \r\n");
			}
		}
  }
  free(t);
}

PROCESS_THREAD(state_machine_process, ev, data)
{
  PROCESS_BEGIN();
  
  {
    // PUT YOUR STATE MACHINE HERE
  }

  // read state machine table length from eeprom
  transLength = eeprom_read_byte((void*)EE_STATEMACHINE_TRANSITIONS); 
  dtTransLength = eeprom_read_byte((void*)EE_STATEMACHINE_DATETIME_TRANSITIONS);

#if STATE_MACHINE_DEBUG
  // output tables so we see if reading it works
  struct transition* tt = malloc(sizeof(struct transition));
  if(tt != NULL)
  {
    int k = 0;
    PRINTF("[State machine table size: %d]:\r\nFrom | Cond | EID | Good | Bad\r\n", transLength);
    for(k = 0;k < transLength;k++)
    {
      eeprom_read_block(tt, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (k * sizeof(struct transition))), sizeof(struct transition));
      PRINTF(" %d | %d | %d | %d | %d \r\n", tt->fromState, tt->cond, tt->eid, tt->goodState, tt->badState);
    }
    PRINTF("[date/time table size: %d]:\r\nFrom | Cond | EID | Good | Bad\r\n", dtTransLength);
    for(k = 0;k < dtTransLength;k++)
    {
      eeprom_read_block(tt, (void*)(1 + EE_STATEMACHINE_DATETIME_TRANSITIONS + (k * sizeof(struct transition))), sizeof(struct transition));
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

  while(1)
  {
    PROCESS_WAIT_EVENT();
    PRINTF("State machine: Received event\r\n");
    PRINTF("state machine: Current state: %d\r\n", curState);
    if(ev == PROCESS_EVENT_TIMER)
    {
      check_datetime_transitions();
      etimer_reset(&check_timer);
    }
    if(ev == sm_data_received_event)
    {
      check_value_transitions(data);
      free(data);

      PRINTF("state machine: Now in state: %d\r\n", curState);
    } 
		if(ev == sm_rulechange_event) {
			// re-read state machine table length from eeprom
  		transLength = eeprom_read_byte((void*)EE_STATEMACHINE_TRANSITIONS); 
  		dtTransLength = eeprom_read_byte((void*)EE_STATEMACHINE_DATETIME_TRANSITIONS);
			PRINTF("State Machine: Re-Reading Table length.\n");
			PRINTF("TransLength: %d, dtTransLength:", transLength, dtTransLength);
		}
  }

  PROCESS_END();
}

