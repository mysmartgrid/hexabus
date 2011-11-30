#include "contiki.h"

#include "hexabus_config.h"
#include "eeprom_variables.h"
#include "../../../../../../shared/hexabus_packet.h"
#include "state_machine.h"
#include "endpoint_access.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>      // memcpy

// #if STATE_MACHINE_DEBUG 
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
// #else
// #define PRINTF(...)
// #define PRINT6ADDR(addr)
// #define PRINTLLADDR(addr)
// #endif

/*------------------------------------------------------*/
PROCESS(state_machine_process, "State Machine Process");
AUTOSTART_PROCESSES(&state_machine_process);
/*------------------------------------------------------*/

static uint8_t transLength; // length of transition table
static uint8_t curState = 0;  // starting out in state 0

bool eval(uint8_t condIndex, struct hxb_data *data) {
  struct condition cond;
  eeprom_read_block(&cond, (void*)(EE_STATEMACHINE_CONDITIONS + (condIndex * sizeof(struct condition))), sizeof(struct condition));

  // Check IPs and EID
  if(memcmp(cond.sourceIP, data->source, 16) || (cond.sourceEID != data->eid))
    return false;

  // Check datatypes, return false if they don't match
  if(data->value.datatype != cond.value.datatype) {
    return false;
  }

  // check the actual condition
  switch(cond.value.datatype)
  {
    case HXB_DTYPE_BOOL:
    case HXB_DTYPE_UINT8:
      if(cond.op == eq)  return data->value.int8 == cond.value.int8;
      if(cond.op == leq) return data->value.int8 <= cond.value.int8;
      if(cond.op == geq) return data->value.int8 >= cond.value.int8;
      if(cond.op == lt)  return data->value.int8 <  cond.value.int8;
      if(cond.op == gt)  return data->value.int8 >  cond.value.int8;
      if(cond.op == neq) return data->value.int8 != cond.value.int8;
      break;
    case HXB_DTYPE_UINT32:
      if(cond.op == eq)  return data->value.int32 == cond.value.int32;
      if(cond.op == leq) return data->value.int32 <= cond.value.int32;
      if(cond.op == geq) return data->value.int32 >= cond.value.int32;
      if(cond.op == lt)  return data->value.int32 <  cond.value.int32;
      if(cond.op == gt)  return data->value.int32 >  cond.value.int32;
      if(cond.op == neq) return data->value.int32 != cond.value.int32;
      break;
    case HXB_DTYPE_FLOAT:
      if(cond.op == eq)  return data->value.float32 == cond.value.float32;
      if(cond.op == leq) return data->value.float32 <= cond.value.float32;
      if(cond.op == geq) return data->value.float32 >= cond.value.float32;
      if(cond.op == lt)  return data->value.float32 <  cond.value.float32;
      if(cond.op == gt)  return data->value.float32 >  cond.value.float32;
      if(cond.op == neq) return data->value.float32 != cond.value.float32;
      break;
    // TODO datetime... aaaargh...
    // TODO maybe we should do datetime as a guard condition, because it's so complicated, and we probably need seperate tests for hour, day, etc.
    // TODO oh, no, we don't need it.. just go into a different state "at 5 o'clock" when you want to do an action "only after 5"
    // TODO for that we still need to check the hours, days, individually.
    default:
      PRINTF("Datatype not implemented in state machine (yet).\r\n");
      return false;
  }
}

PROCESS_THREAD(state_machine_process, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("State Machine starting.\r\n");

  // read state machine tables from EEPROM
  transLength = eeprom_read_byte((void*)EE_STATEMACHINE_TRANSITIONS); 

  // output table so we see if reading it works
  struct transition* t = malloc(sizeof(struct transition));
  if(t != NULL)
  {
    int k = 0;
    PRINTF("[State machine table size: %d]: From | Cond | EID | Good | Bad\r\n", transLength);
    for(k = 0;k < transLength;k++)
    {
      eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (k * sizeof(struct transition))), sizeof(struct transition));
      PRINTF(" %d | %d | %d | %d | %d \r\n", t->fromState, t->cond, t->eid, t->goodState, t->badState);
    }
  } else {
    PRINTF("malloc failed!\r\n");
  }
  free(t);

  while(1)
  {
    PROCESS_WAIT_EVENT();
    if(ev == sm_data_received_event)
    {
      PRINTF("State machine: Received event\r\n");

      struct hxb_data *edata = (struct hxb_data*)data;
      struct transition* t = malloc(sizeof(struct transition));
      uint8_t i;
      for(i = 0;i < transLength;i++)
      {
        eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
        if((t->fromState == curState) && (eval(t->cond, edata)))
        {
          // Match found
          PRINTF("state_machine: Writing to endpoint %d \r\n", t->eid);
          if(endpoint_write(t->eid, &(t->data)) == 0)
          {
            curState = t->goodState;
            PRINTF("state_machine: Everything is fine \r\n");
            break;
          } else {                          // Something went wrong
            curState = t->badState;
            PRINTF("state_machine: Something bad happened \r\n");
            break;
          }
        }
      }
      free(t);
      free(data);
    }
  }

  PROCESS_END();
}

