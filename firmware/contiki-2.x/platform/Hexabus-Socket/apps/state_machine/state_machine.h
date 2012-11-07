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

// Internal stuff: these structs implement a simple table layout

// op for datetime: bits 0..6 denote dependency on hour, minute, second, ...; bit 7 sets whether to check for >= or <.
// date/time transitions need to be stored separately. They are also executed seperately, each time before the "normal" transitions are executed

// Used for wrapping the eeprom access

#define SM_CONDITION_LENGTH 0
#define SM_TRANSITION_LENGTH 1
#define SM_DATETIME_TRANSITION_LENGTH 2

// Defintion of events that are important to the state machine
// One general event for all data that can be possibly received
extern process_event_t sm_data_received_event;

uint8_t sm_is_running();
void sm_restart();
void sm_start();
void sm_stop();

#endif /* STATE_MACHINE_H_*/
