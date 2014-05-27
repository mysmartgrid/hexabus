#include "state_machine_eeprom.h"
#include <util/delay.h>
#include <avr/interrupt.h>

void sm_get_id(char* b) {
	eeprom_read_block(b, eep_addr(sm.id), eep_size(sm.id));
}

bool sm_id_is_zero() {
  char sm_id[eep_size(sm.id)];
  sm_get_id(sm_id);
  uint8_t sum = 0;
  for(size_t i = 0; i < sizeof(sm_id); i++) {
    sum |= sm_id[i];
  }

  return (sum == 0);
}

uint8_t sm_get_number_of_conditions() {
	return eeprom_read_byte(eep_addr(sm.n_conditions));
}

void sm_set_number_of_conditions(uint8_t number) {
	eeprom_update_byte(eep_addr(sm.n_conditions), number);
}

uint8_t sm_get_number_of_transitions(bool datetime) {
	return datetime
		? eeprom_read_byte(eep_addr(sm.n_dt_transitions))
		: eeprom_read_byte(eep_addr(sm.n_transitions));
}

void sm_set_number_of_transitions(bool datetime, uint8_t number) {
	if (datetime)
		eeprom_update_byte(eep_addr(sm.n_dt_transitions), number);
	else 
		eeprom_update_byte(eep_addr(sm.n_transitions), number);
}

void sm_write_condition(uint8_t index, struct condition *cond) {
	eeprom_update_block(cond, eep_addr(sm.conditions[sizeof(struct condition) * index]), sizeof(struct condition));
}

void sm_get_condition(uint8_t index, struct condition *cond) {
	eeprom_read_block(cond, eep_addr(sm.conditions[sizeof(struct condition) * index]), sizeof(struct condition));
}

void sm_set_id(char* b) {
	eeprom_update_block(b, eep_addr(sm.id), eep_size(sm.id));
}

void sm_write_transition(bool datetime, uint8_t index, struct transition *trans) {
	if (datetime)
		eeprom_update_block(trans, eep_addr(sm.dt_transitions[sizeof(struct transition) * index]), sizeof(struct transition));
	else
		eeprom_update_block(trans, eep_addr(sm.transitions[sizeof(struct transition) * index]), sizeof(struct transition));
}

void sm_get_transition(bool datetime, uint8_t index, struct transition *trans) {
	if (datetime)
		eeprom_read_block(trans, eep_addr(sm.dt_transitions[sizeof(struct transition) * index]), sizeof(struct transition));
	else
		eeprom_read_block(trans, eep_addr(sm.transitions[sizeof(struct transition) * index]), sizeof(struct transition));
}

bool sm_write_chunk(uint8_t chunk_id, char* data) {
	// check where the chunk goes: the first chunk goes to the domain_name area of the eeprom
	if(chunk_id == 0) {
		if(data[0] == '\0') // if the first byte of the new name is zero, don't write anything (leave the old name in the eeprom)
			return true;
		cli();
		eeprom_update_block(data, eep_addr(domain_name), eep_size(domain_name));
		eeprom_busy_wait();
		sei();
		// The write fails if no function call follows an EEPROM write. This is strangeness.
		_delay_ms(2);
		return true;
	} else { // the other chunks go to the statemachine area
		if (chunk_id * EE_STATEMACHINE_CHUNK_SIZE > sizeof(struct hxb_sm_eeprom_layout))
			return false;

		chunk_id -= 1; // subtract 1 from chunk id, because the first chunk went to the DOMAIN_NAME, so we have to restart counting from 0
		// check if chunk offset is valid
		cli();

		char* sm_base_addr = eep_addr(sm);
		eeprom_update_block(data, sm_base_addr + chunk_id * EE_STATEMACHINE_CHUNK_SIZE, EE_STATEMACHINE_CHUNK_SIZE);
		eeprom_busy_wait();
		sei();
		// The write fails if no function call follows an EEPROM write. This is strangeness.
		_delay_ms(2);
		return true;
	}
}
