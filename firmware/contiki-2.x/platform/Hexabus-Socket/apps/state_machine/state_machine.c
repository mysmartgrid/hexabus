#include "contiki.h"

#include "../../../../../../shared/hexabus_packet.h"
#include "state_machine.h"
#include "endpoint_access.h"
#include <stdbool.h>
#include <stdlib.h>

#define DEBUG 1
#if DEBUG
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

static struct condition *condTable;
static struct transition *transTable;		
static uint8_t transLength = 2;							// length of transition table
static uint8_t curState = 0;						// starting out in state 0

bool eval(uint8_t condIndex, struct hxb_data *data) {
	struct condition *cond = &condTable[condIndex];
	// TODO: Check for IP
	if(cond->sourceEID != data->eid) {
		return false;
	}
	switch(cond->op) {
		case eq:
			return (cond->value == data->value.int8);
		case leq:
			return (cond->value <= data->value.int8);
		case geq:
			return (cond->value >= data->value.int8);
		case lt:
			return (cond->value < data->value.int8);
		case gt:
			return (cond->value > data->value.int8);
		default:
			return false;
	}
}

PROCESS_THREAD(state_machine_process, ev, data)
{
	  
	PROCESS_BEGIN();
  PRINTF("State Machine starting.");
/* Definition of an actual state machine. Simple relay-Trigger */
	condTable = malloc(sizeof(struct condition));
	transTable = malloc(sizeof(struct transition));
	struct hxb_value wData;
	wData.datatype = HXB_DTYPE_BOOL;
	wData.int8 = HXB_TRUE;

	condTable[0].sourceIP = 0;
	condTable[0].sourceEID = 1;
	condTable[0].op = eq;
	condTable[0].value = 10;

	transTable[0].fromState = 0;
	transTable[0].cond = 0;
	transTable[0].eid = 1;
	memcpy(&(transTable[0].data), &wData, sizeof(struct hxb_value));
	transTable[0].goodState = 1;
	transTable[0].badState = 0;
	transTable[1].fromState = 1;
	transTable[1].cond = 0;
	transTable[1].eid = 1;
	memcpy(&(transTable[1].data), &wData, sizeof(struct hxb_value));
	transTable[1].goodState = 0;
	transTable[1].badState = 1;
	/*----------------------------------------*/

  while(1)
  {
    PROCESS_WAIT_EVENT();
  	if(ev == sm_data_received_event) {
			// something happened, better check our own tables
			struct hxb_data *edata = (struct hxb_data*)data;
			uint8_t i;
			for(i = 0;i < transLength;i++) {
				if((transTable[i].fromState == curState) && (eval(transTable[i].cond, edata))) {
					// Match found
					printf("state_machine: Writing to endpoint %d \r\n", transTable[i].eid);
					if(endpoint_write(transTable[i].eid, &(transTable[i].data)) == 0) {			
						curState = transTable[i].goodState;
						printf("state_machine: Everything is fine \r\n");
						break;
					} else {													// Something went wrong
						curState = transTable[i].badState;
						printf("state_machine: Something bad happened \r\n");
						break;
					}
				}
			}
		}
	}

	PROCESS_END();
}

