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

static struct condition *condTable;
static struct transition *transTable;
static uint8_t transLength = 2;              // length of transition table
static uint8_t curState = 0;            // starting out in state 0

bool eval(uint8_t condIndex, struct hxb_data *data) {
  struct condition *cond = &condTable[condIndex];

  // Check IPs and EID
  if(memcmp(cond->sourceIP, data->source, 16) || (cond->sourceEID != data->eid))
    return false;

  // Check datatypes, return false if they don't match
  if(data->value.datatype != cond->value.datatype) {
    return false;
  }

  // check the actual condition
  switch(cond->value.datatype)
  {
    case HXB_DTYPE_BOOL:
    case HXB_DTYPE_UINT8:
      if(cond->op == eq)  return data->value.int8 == cond->value.int8;
      if(cond->op == leq) return data->value.int8 <= cond->value.int8;
      if(cond->op == geq) return data->value.int8 >= cond->value.int8;
      if(cond->op == lt)  return data->value.int8 <  cond->value.int8;
      if(cond->op == gt)  return data->value.int8 >  cond->value.int8;
      if(cond->op == neq) return data->value.int8 != cond->value.int8;
      break;
    case HXB_DTYPE_UINT32:
      if(cond->op == eq)  return data->value.int32 == cond->value.int32;
      if(cond->op == leq) return data->value.int32 <= cond->value.int32;
      if(cond->op == geq) return data->value.int32 >= cond->value.int32;
      if(cond->op == lt)  return data->value.int32 <  cond->value.int32;
      if(cond->op == gt)  return data->value.int32 >  cond->value.int32;
      if(cond->op == neq) return data->value.int32 != cond->value.int32;
      break;
    case HXB_DTYPE_FLOAT:
      if(cond->op == eq)  return data->value.float32 == cond->value.float32;
      if(cond->op == leq) return data->value.float32 <= cond->value.float32;
      if(cond->op == geq) return data->value.float32 >= cond->value.float32;
      if(cond->op == lt)  return data->value.float32 <  cond->value.float32;
      if(cond->op == gt)  return data->value.float32 >  cond->value.float32;
      if(cond->op == neq) return data->value.float32 != cond->value.float32;
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
  condTable = malloc(sizeof(struct condition));
  transTable = malloc(transLength*sizeof(struct transition));

  // read state machine tables from EEPROM
  eeprom_read_block(condTable, (void*)EE_STATEMACHINE_CONDITIONS, sizeof(struct condition));
  eeprom_read_block(transTable, (void*)EE_STATEMACHINE_TRANSITIONS, sizeof(struct transition) * transLength);

  /* struct hxb_value wData;
  wData.datatype = HXB_DTYPE_BOOL;
  wData.int8 = HXB_FALSE;

  condTable[0].sourceIP[0] = 0xfe;
  condTable[0].sourceIP[1] = 0x80;
  condTable[0].sourceIP[2] = 0x00;
  condTable[0].sourceIP[3] = 0x00;
  condTable[0].sourceIP[4] = 0x00;
  condTable[0].sourceIP[5] = 0x00;
  condTable[0].sourceIP[6] = 0x00;
  condTable[0].sourceIP[7] = 0x00;
  condTable[0].sourceIP[8] = 0x00;
  condTable[0].sourceIP[9] = 0x50;
  condTable[0].sourceIP[10] = 0xc4;
  condTable[0].sourceIP[11] = 0xff;
  condTable[0].sourceIP[12] = 0xfe;
  condTable[0].sourceIP[13] = 0x04;
  condTable[0].sourceIP[14] = 0x00;
  condTable[0].sourceIP[15] = 0x0e;

  condTable[0].sourceEID = 1;
  condTable[0].op = eq;
  condTable[0].value.datatype = HXB_DTYPE_UINT8;
  condTable[0].value.int8 = 10;

  transTable[0].fromState = 0;
  transTable[0].cond = 0;
  transTable[0].eid = 1;
  memcpy(&(transTable[0].data), &wData, sizeof(struct hxb_value));
  transTable[0].goodState = 1;
  transTable[0].badState = 0;
  transTable[1].fromState = 1;
  transTable[1].cond = 0;
  transTable[1].eid = 1;
  wData.int8 = HXB_TRUE;
  memcpy(&(transTable[1].data), &wData, sizeof(struct hxb_value));
  transTable[1].goodState = 0;
  transTable[1].badState = 1;

  // save data to eeprom
  eeprom_write_block(condTable, (void*)EE_STATEMACHINE_CONDITIONS, sizeof(struct condition));
  eeprom_write_block(transTable, (void*)EE_STATEMACHINE_TRANSITIONS, transLength * sizeof(struct transition)); */

  /* DEBUG */
  int k = 0;
  PRINTF("[State Machine Table]: From | Cond | EID | Good | Bad\r\n");
  for(k = 0;k < transLength;k++) {
    PRINTF(" %d | %d | %d | %d | %d \r\n", transTable[k].fromState, transTable[k].cond, transTable[k].eid, transTable[k].goodState, transTable[k].badState);
  }
  /* END */

  while(1)
  {
    PROCESS_WAIT_EVENT();
    if(ev == sm_data_received_event)
    {
      PRINTF("State machine: Received event\r\n");

      struct hxb_data *edata = (struct hxb_data*)data;
      uint8_t i;
      for(i = 0;i < transLength;i++)
      {
        if((transTable[i].fromState == curState) && (eval(transTable[i].cond, edata)))
        {
          // Match found
          PRINTF("state_machine: Writing to endpoint %d \r\n", transTable[i].eid);
          if(endpoint_write(transTable[i].eid, &(transTable[i].data)) == 0)
          {
            curState = transTable[i].goodState;
            PRINTF("state_machine: Everything is fine \r\n");
            break;
          } else {                          // Something went wrong
            curState = transTable[i].badState;
            PRINTF("state_machine: Something bad happened \r\n");
            break;
          }
        }
      }

      free(data);
    }
  }

  PROCESS_END();
}

