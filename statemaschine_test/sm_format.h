//sm_format.h Defines the table format of the state maschine. 

#include <stdint.h>

// Operators
enum Oper { eq, leq, geq, lt, gt };

struct Condition {
	uint8_t ip; 		// FIXME IP
	uint8_t eid; 		// Endpoint ID
	Oper op; 		    // predicate function
	uint8_t value; 		// the constant the received value will be checked against
	
};

struct Transition {
	uint8_t fromState; 	// The State the maschine is currently in
	Condition *cond; 		// The Condition that must match for the transition
	uint8_t action; 		// Function pointer
	uint8_t toCool; 		// New State if function was executed successfully
	uint8_t toUncool; 	// New State if function was _not_ executed successfully
};

