/*
 * state_machine.h
 */

#ifndef STATE_MACHINE_H_ 
#define STATE_MACHINE_H_

#include "process.h"
#include <stdint.h>
#include "../../../../../shared/hexabus_packet.h"

PROCESS_NAME(state_machine_process);

// Internal stuff: these structs implement a simple table layout

// Operators for comparison
enum oper { eq, leq, geq, lt, gt };

struct condition {
  char sourceIP[16];      // TODO: IP
  uint8_t sourceEID;      // EID we expect data from
  enum oper op;           // predicate function
  struct hxb_value value; // the constant to compare with
};

struct transition {
  uint8_t fromState;      // current state
  uint8_t cond;           // index of condition that must be matched
  uint8_t eid;            // id of endpoint which should do something
  uint8_t goodState;      // new state if everything went fine
  uint8_t badState;       // new state if some went wrong
  struct hxb_value data;  // Data for the endpoint
};


// Defintion of events that are important to the state machine
// One general event for all data that can be possibly received
extern process_event_t sm_data_received_event;

#endif /* STATE_MACHINE_H_*/
