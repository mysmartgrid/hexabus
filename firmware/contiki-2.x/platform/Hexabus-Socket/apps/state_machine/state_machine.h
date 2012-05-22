/*
 * state_machine.h
 */

#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "process.h"
#include <stdint.h>
#include "../../../../../shared/hexabus_packet.h"
#include "../../../../../shared/hexabus_statemachine_structs.h"

PROCESS_NAME(state_machine_process);

// Defintion of events that are important to the state machine
// One general event for all data that can be possibly received
extern process_event_t sm_data_received_event;
extern process_event_t sm_rulechange_event;
#endif /* STATE_MACHINE_H_*/
