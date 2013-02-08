#ifndef APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H
#define APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "eeprom_variables.h"
#include "../../../../../shared/hexabus_packet.h"
#include "../../../../../shared/hexabus_statemachine_structs.h"

// Used for wrapping the eeprom access
//
//#define SM_CONDITION_LENGTH 0
//#define SM_TRANSITION_LENGTH 1
//#define SM_DATETIME_TRANSITION_LENGTH 2
//

// State Machine eeprom access
void sm_get_id(char* b);
bool sm_id_is_zero();
uint8_t sm_get_number_of_conditions();
void sm_set_number_of_conditions(uint8_t number);
uint8_t sm_get_number_of_transitions(bool datetime);
void sm_set_id(char* b);
void sm_set_number_of_transitions(bool datetime, uint8_t number);
void sm_write_condition(uint8_t index, struct condition *cond);
void sm_get_condition(uint8_t index, struct condition *cond);
void sm_write_transition(bool datetime, uint8_t index, struct transition *trans);
void sm_get_transition(bool datetime, uint8_t index, struct transition *trans);

bool sm_write_chunk(uint8_t chunk_id, char* data);

#endif /* APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H */

