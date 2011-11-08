#include "contiki.h"

#include "../../../../../../shared/hexabus_packet.h"
#include "state_machine.h"
#include "endpoint_access.h"
#include <stdbool.h>

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
/* Definition of an actual state machine */
//condTable = malloc(sizeof(condition));
/*----------------------------------------*/

bool eval(uint8_t condIndex, struct hxb_value *value, uint8_t eid) {
	struct condition *cond = &condTable[condIndex];
	// TODO: Check for IP
	if(cond->targetEID != eid) {
		return false;
	}
	
	switch(cond->op) {
		case eq:
			return (cond->value == value->int8);
		case leq:
			return (cond->value <= value->int8);
		case geq:
			return (cond->value >= value->int8);
		case lt:
			return (cond->value < value->int8);
		case gt:
			return (cond->value > value->int8);
		default:
			return false;
	}
}

PROCESS_THREAD(state_machine_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("State Machine starting.");

  while(1)
  {
    PROCESS_WAIT_EVENT();
  	if(ev == sm_data_received_event) {
			// something happened, better check our own tables
			//struct sm_data *receivedData = (struct sm_data*)data;			// TODO: casting ok?
			struct hxb_value *value = (struct hxb_value*)data;
			uint8_t i;
			for(i = 0;i < transLength;i++) {
				if((transTable[i].fromState == curState) && (eval(transTable[i].cond, value, 1))) {
					// Match found
					printf("Writing to endpoint %d", transTable[i].action);
					if(endpoint_write(1, value) == 0) {			// Everything went fine
						curState = transTable[i].goodState;
						printf("Everything is fine");
						break;
					} else {													// Something went wrong
						curState = transTable[i].badState;
						printf("Something bad happened");
						break;
					}
				}
			}
		}
	}

	PROCESS_END();
}

