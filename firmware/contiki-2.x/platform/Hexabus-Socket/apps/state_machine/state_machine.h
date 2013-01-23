/*
 * state_machine.h
 */

#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "process.h"
#include <stdint.h>
#include "state_machine_eeprom.h"
#include "../../../../../shared/hexabus_packet.h"
#include "../../../../../shared/hexabus_statemachine_structs.h"

PROCESS_NAME(state_machine_process);

// Defintion of events that are important to the state machine
// One general event for all data that can be possibly received
extern process_event_t sm_data_received_event;

uint8_t sm_is_running();
void sm_restart(); // silent: restart state machine without sending a reset broadcast (used when restarted by a reset broadcast)
void sm_start();
void sm_stop();

#endif /* STATE_MACHINE_H_*/
