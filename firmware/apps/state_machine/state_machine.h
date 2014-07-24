/*
 * state_machine.h
 */

#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "process.h"
#include <stdint.h>
#include "state_machine_eeprom.h"
#include "hexabus_packet.h"
#include "hexabus_statemachine_structs.h"

uint8_t sm_is_running();
void sm_restart(); // silent: restart state machine without sending a reset broadcast (used when restarted by a reset broadcast)
void sm_start();
void sm_stop();

void sm_handle_input(struct hxb_envelope* data);

#endif /* STATE_MACHINE_H_*/
