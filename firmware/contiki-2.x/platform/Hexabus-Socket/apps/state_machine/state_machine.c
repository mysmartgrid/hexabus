#include "contiki.h"

#include "hexabus_config.h"
#include "../../../../../../shared/hexabus_packet.h"
#include "state_machine.h"
#include "endpoint_access.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>			// memcpy

#if STATE_MACHINE_DEBUG 
#include <stdio.h>
#define PRINTF(...) PRINTF(__VA_ARGS__)
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

static struct condition *condTable;
static struct transition *transTable;		
static uint8_t transLength = 2;							// length of transition table
static uint8_t curState = 0;						// starting out in state 0

bool eval(uint8_t condIndex, struct hxb_data *data) {
	struct condition *cond = &condTable[condIndex];
	
	// Check IPs and EID
	bool null = true;
	bool equal = true;
	uint8_t k = 0;
	for(k = 0;k < 16;k++) {
		if(cond->sourceIP[k] != 0) {
			null = false;
		}
		if(cond->sourceIP[k] != data->source[k]) {
			equal = false;
		}
	}
	
	if((!null && !equal) || (cond->sourceEID != data->eid)) {
		return false;
	}
	
	// Now check for the actual data. first of all, we have to check for compatible datatypes
	if(data->value.datatype != cond->value.datatype) {
		return false;
	}

	// Now lets check for the operator and then for datatypes. TODO: Datetime comperator
	switch(cond->op) {
		case eq:
			if((cond->value.datatype == HXB_DTYPE_UINT8) || (cond->value.datatype == HXB_DTYPE_BOOL))
				return (cond->value.int8 == data->value.int8);
			if(cond->value.datatype == HXB_DTYPE_UINT32)
				return (cond->value.int32 == data->value.int32);
		case leq:
			if(cond->value.datatype == HXB_DTYPE_UINT8)
				return (cond->value.int8 <= data->value.int8);
			if(cond->value.datatype == HXB_DTYPE_UINT32)
				return (cond->value.int32 <= data->value.int32);
		case geq:
			if(cond->value.datatype == HXB_DTYPE_UINT8)
				return (cond->value.int8 >= data->value.int8);
			if(cond->value.datatype == HXB_DTYPE_UINT32)
				return (cond->value.int32 >= data->value.int32);
		case lt:
			if(cond->value.datatype == HXB_DTYPE_UINT8)
				return (cond->value.int8 < data->value.int8);
			if(cond->value.datatype == HXB_DTYPE_UINT32)
				return (cond->value.int32 < data->value.int32);
		case gt:
			if(cond->value.datatype == HXB_DTYPE_UINT8)
				return (cond->value.int8 > data->value.int8);
			if(cond->value.datatype == HXB_DTYPE_UINT32)
				return (cond->value.int32 > data->value.int32);
		default:
			return false;
	}
}

PROCESS_THREAD(state_machine_process, ev, data)
{
	  
	PROCESS_BEGIN();
  PRINTF("State Machine starting.\r\n");
	/* Definition of an actual state machine. Simple relay-Trigger */
	condTable = malloc(sizeof(struct condition));
	transTable = malloc(transLength*sizeof(struct transition));
	struct hxb_value wData;
	wData.datatype = HXB_DTYPE_BOOL;
	wData.int8 = HXB_FALSE;

	uint8_t a = 0;
	for(a = 0;a < 16;a++) {
		condTable[0].sourceIP[a] = 0;
	}
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
	/*----------------------------------------*/

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
			// something happened, better check our own tables
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
					} else {													// Something went wrong
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

