/*
 * state_machine.h
 */

#ifndef STATE_MACHINE_H_ 
#define STATE_MACHINE_H_

#include "process.h"
#include <stdint.h>

// Operators for comparison
enum Oper { eq, leq, geq, lt, gt};

// these structs implement a simple table layout

struct Condition {
	uint8_t ip;					// TODO: IP
	uint8_t eid;				// local endpoint id
	enum Oper op;				// predicate function
	uint8_t value;			// the constant to compare with
};

struct Transition {
	uint8_t fromState;	// current state
	uint8_t cond;				// index of condition that must be matched
	uint8_t action;			// id of endpoint which should do something
	uint8_t goodState;	// new state if everything went fine
	uint8_t badState;		// new state if some went wrong
};

PROCESS_NAME(state_machine_process);

// Defintion of events that are important to the state machine
// One general event for all data that can be possibly received
extern process_event_t data_received_event;
// extern process_event_t new_config_event;

#endif /* STATE_MACHINE_H_*/
