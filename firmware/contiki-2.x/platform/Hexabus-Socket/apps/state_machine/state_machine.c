#include "contiki.h"

#include "../../../../../../shared/hexabus_packet.h"
#include "state_machine.h"
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

static struct Condition *condTable;
static struct Transition *transTable;		
static uint8_t transLength;							// length of transition table
static uint8_t curState = 0;						// starting out in state 0

bool eval(uint8_t condIndex, uint8_t value) {
	struct Condition *cond = &condTable[condIndex];
	// TODO: Check for IP&EID
	
	switch(cond->op) {
		case eq:
			return (cond->value == value);
		case leq:
			return (cond->value <= value);
		case geq:
			return (cond->value >= value);
		case lt:
			return (cond->value < value);
		case gt:
			return (cond->value > value);
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
  	// something happened, better check our own tables
		uint8_t i;
		uint8_t value = 0;
		for(i = 0;i < transLength;i++) {
			if((transTable[i].fromState == curState) && (eval(transTable[i].cond, value))) {
				// Match found
				printf("Executing function number %d", transTable[i].action);
				// TODO: Do actual stuff using endpoint access
				curState = transTable[i].goodState;
				break;
			}
		}
	
	}

	PROCESS_END();
}

