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
  // get the condition from eeprom
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
      if(cond.op == STM_EQ)  return data->value.int8 == cond.value.int8;
      if(cond.op == STM_LEQ) return data->value.int8 <= cond.value.int8;
      if(cond.op == STM_GEQ) return data->value.int8 >= cond.value.int8;
      if(cond.op == STM_LT)  return data->value.int8 <  cond.value.int8;
      if(cond.op == STM_GT)  return data->value.int8 >  cond.value.int8;
      if(cond.op == STM_NEQ) return data->value.int8 != cond.value.int8;
      break;
    case HXB_DTYPE_UINT32:
      if(cond.op == STM_EQ)  return data->value.int32 == cond.value.int32;
      if(cond.op == STM_LEQ) return data->value.int32 <= cond.value.int32;
      if(cond.op == STM_GEQ) return data->value.int32 >= cond.value.int32;
      if(cond.op == STM_LT)  return data->value.int32 <  cond.value.int32;
      if(cond.op == STM_GT)  return data->value.int32 >  cond.value.int32;
      if(cond.op == STM_NEQ) return data->value.int32 != cond.value.int32;
      break;
    case HXB_DTYPE_FLOAT:
      if(cond.op == STM_EQ)  return data->value.float32 == cond.value.float32;
      if(cond.op == STM_LEQ) return data->value.float32 <= cond.value.float32;
      if(cond.op == STM_GEQ) return data->value.float32 >= cond.value.float32;
      if(cond.op == STM_LT)  return data->value.float32 <  cond.value.float32;
      if(cond.op == STM_GT)  return data->value.float32 >  cond.value.float32;
      if(cond.op == STM_NEQ) return data->value.float32 != cond.value.float32;
      break;
    case HXB_DTYPE_DATETIME:
      if(cond.op & 0x01) // hour
        return cond.op & 0x80 ? data->value.datetime.hour >= cond.value.datetime.hour : data->value.datetime.hour < cond.value.datetime.hour;
      if(cond.op & 0x02) // minute
        return cond.op & 0x80 ? data->value.datetime.minute >= cond.value.datetime.minute : data->value.datetime.minute < cond.value.datetime.minute;
      if(cond.op & 0x04) // second
        return cond.op & 0x80 ? data->value.datetime.second >= cond.value.datetime.second : data->value.datetime.second < cond.value.datetime.second;
      if(cond.op & 0x08) // day
        return cond.op & 0x80 ? data->value.datetime.day >= cond.value.datetime.day : data->value.datetime.day < cond.value.datetime.day;
      if(cond.op & 0x10) // month
        return cond.op & 0x80 ? data->value.datetime.month >= cond.value.datetime.month : data->value.datetime.month < cond.value.datetime.month;
      if(cond.op & 0x20) // year
        return cond.op & 0x80 ? data->value.datetime.year >= cond.value.datetime.year : data->value.datetime.year < cond.value.datetime.year;
      if(cond.op & 0x40) // weekday
        return cond.op & 0x80 ? data->value.datetime.weekday >= cond.value.datetime.weekday : data->value.datetime.weekday < cond.value.datetime.weekday;
      break;
    default:
      PRINTF("Datatype not implemented in state machine (yet).\r\n");
      return false;
  }
}

PROCESS_THREAD(state_machine_process, ev, data)
{
  PROCESS_BEGIN();

  /*  TESTING INIT (((o) (o)))
                        v
  */
  // clear the eeprom

  for(int i = 0 ; i < 127 ; i++)
  {
    int zero = 0;
    eeprom_write_block(&zero, (void*)(EE_STATEMACHINE_TRANSITIONS + i), 1);
    eeprom_write_block(&zero, (void*)(EE_STATEMACHINE_CONDITIONS + i), 1);
    eeprom_write_block(&zero, (void*)(EE_STATEMACHINE_DATETIME_CONDITIONS + i), 1); // not yet implemented or thought through
  }

  // write transitions into the EEPROM
  {
    int numberOfTransitions = 2;
    eeprom_write_block(&numberOfTransitions, (void*)EE_STATEMACHINE_TRANSITIONS, 1);
    struct transition trans;
    trans.fromState = 0;
    trans.cond = 0; // condition index
    trans.eid = 1;
    trans.goodState = 1;
    trans.badState = 0;
    trans.data.datatype = HXB_DTYPE_BOOL;
    trans.data.int8 = HXB_TRUE;
    eeprom_write_block(&trans, (void*)(1 + EE_STATEMACHINE_TRANSITIONS), sizeof(struct transition));

    trans.fromState = 1;
    trans.cond = 0; // condition index
    trans.eid = 1;
    trans.goodState = 0;
    trans.badState = 1;
    trans.data.datatype = HXB_DTYPE_BOOL;
    trans.data.int8 = HXB_FALSE;
    eeprom_write_block(&trans, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (sizeof(struct transition))), sizeof(struct transition));

    struct condition cond;
    cond.sourceIP[0] = 0xfe;
    cond.sourceIP[1] = 0x80;
    cond.sourceIP[2] = 0x00;
    cond.sourceIP[3] = 0x00;
    cond.sourceIP[4] = 0x00;
    cond.sourceIP[5] = 0x00;
    cond.sourceIP[6] = 0x00;
    cond.sourceIP[7] = 0x00;
    cond.sourceIP[8] = 0x00;
    cond.sourceIP[9] = 0x50;
    cond.sourceIP[10] = 0xc4;
    cond.sourceIP[11] = 0xff;
    cond.sourceIP[12] = 0xfe;
    cond.sourceIP[13] = 0x04;
    cond.sourceIP[14] = 0x00;
    cond.sourceIP[15] = 0x0e;
    cond.sourceEID = 2;
    cond.op = STM_EQ;
    cond.value.datatype = HXB_DTYPE_UINT8;
    cond.value.int8 = 10;
    eeprom_write_block(&cond, (void*)EE_STATEMACHINE_CONDITIONS, sizeof(struct condition));
  }
  // **************************


  PRINTF("State Machine starting.\r\n");

  // read state machine table length from eeprom
  transLength = eeprom_read_byte((void*)EE_STATEMACHINE_TRANSITIONS); 

  // output table so we see if reading it works
  struct transition* t = malloc(sizeof(struct transition));
  if(t != NULL)
  {
    int k = 0;
    PRINTF("[State machine table size: %d]:\r\nFrom | Cond | EID | Good | Bad\r\n", transLength);
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
        // check value-dependet transitions
        eeprom_read_block(t, (void*)(1 + EE_STATEMACHINE_TRANSITIONS + (i * sizeof(struct transition))), sizeof(struct transition));
        // get next transition to check from eeprom
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

