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
// TODO this is 2 bytes in memory. Maybe we can shorten it to one byte!
// Changing it to defines now so we can also use the op for datetimey-things. Leaving it at 16 bit to not break the eeprom format until we have a proper programming interface
#define STM_EQ   0x0000
#define STM_LEQ  0x0001
#define STM_GEQ  0x0002
#define STM_LT   0x0003
#define STM_GT   0x0004
#define STM_NEQ  0x0005

// op for datetime: bits 0..6 denote dependency on hour, minute, second, ...; bit 7 sets whether to check for >= or <.

struct condition {
  uint8_t sourceIP[16];      // IP
  uint8_t sourceEID;      // EID we expect data from
  uint16_t op;           // predicate function
  struct hxb_value value; // the constant to compare with
} __attribute__ ((packed));

struct transition {
  uint8_t fromState;      // current state
  uint8_t cond;           // index of condition that must be matched
  uint8_t eid;            // id of endpoint which should do something
  uint8_t goodState;      // new state if everything went fine
  uint8_t badState;       // new state if some went wrong
  struct hxb_value data;  // Data for the endpoint
} __attribute__ ((packed));


// Defintion of events that are important to the state machine
// One general event for all data that can be possibly received
extern process_event_t sm_data_received_event;

#endif /* STATE_MACHINE_H_*/
