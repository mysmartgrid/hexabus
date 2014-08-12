#include "state_machine_eeprom.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include "nvm.h"

void sm_get_id(char* b) {
	nvm_read_block(sm.id, b, nvm_size(sm.id));
}

bool sm_id_is_zero() {
  char sm_id[nvm_size(sm.id)];
  sm_get_id(sm_id);
  uint8_t sum = 0;
  for(size_t i = 0; i < sizeof(sm_id); i++) {
    sum |= sm_id[i];
  }

  return (sum == 0);
}

uint8_t sm_get_number_of_conditions() {
	return nvm_read_u8(sm.n_conditions);
}

void sm_set_number_of_conditions(uint8_t number) {
	nvm_write_u8(sm.n_conditions, number);
}

uint8_t sm_get_number_of_transitions(bool datetime) {
	return datetime
		? nvm_read_u8(sm.n_dt_transitions)
		: nvm_read_u8(sm.n_transitions);
}

void sm_set_number_of_transitions(bool datetime, uint8_t number) {
	if (datetime)
		nvm_write_u8(sm.n_dt_transitions, number);
	else
		nvm_write_u8(sm.n_transitions, number);
}

void sm_write_condition(uint8_t index, struct condition *cond) {
	nvm_write_block(sm.conditions[sizeof(struct condition) * index], cond, sizeof(struct condition));
}

void sm_get_condition(uint8_t index, struct condition *cond) {
	nvm_read_block(sm.conditions[sizeof(struct condition) * index], cond, sizeof(struct condition));
}

void sm_set_id(char* b) {
	nvm_write_block(sm.id, b, nvm_size(sm.id));
}

void sm_write_transition(bool datetime, uint8_t index, struct transition *trans) {
	if (datetime)
		nvm_write_block(sm.dt_transitions[sizeof(struct transition) * index], trans, sizeof(struct transition));
	else
		nvm_write_block(sm.transitions[sizeof(struct transition) * index], trans, sizeof(struct transition));
}

void sm_get_transition(bool datetime, uint8_t index, struct transition *trans) {
	if (datetime)
		nvm_read_block(sm.dt_transitions[sizeof(struct transition) * index], trans, sizeof(struct transition));
	else
		nvm_read_block(sm.transitions[sizeof(struct transition) * index], trans, sizeof(struct transition));
}

bool sm_write_chunk(uint8_t chunk_id, char* data) {
	// check where the chunk goes: the first chunk goes to the domain_name area of the eeprom
	if(chunk_id == 0) {
		if(data[0] == '\0') // if the first byte of the new name is zero, don't write anything (leave the old name in the eeprom)
			return true;
		nvm_write_block(domain_name, data, nvm_size(domain_name));
		return true;
	} else { // the other chunks go to the statemachine area
		if (chunk_id * EE_STATEMACHINE_CHUNK_SIZE > sizeof(struct hxb_sm_nvm_layout))
			return false;

		chunk_id -= 1; // subtract 1 from chunk id, because the first chunk went to the DOMAIN_NAME, so we have to restart counting from 0
		// check if chunk offset is valid
		nvm_write_block_at(nvm_addr(sm) + chunk_id * EE_STATEMACHINE_CHUNK_SIZE, data, EE_STATEMACHINE_CHUNK_SIZE);
		return true;
	}
}
