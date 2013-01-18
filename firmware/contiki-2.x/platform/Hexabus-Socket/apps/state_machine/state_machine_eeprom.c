#include "state_machine_eeprom.h"
#include <util/delay.h>
#include <avr/interrupt.h>

uint8_t sm_get_id() {
  return eeprom_read_byte ((const void*)EE_STATEMACHINE_ID);
}

uint8_t sm_get_number_of_conditions() {
	return eeprom_read_byte ((const void *)EE_STATEMACHINE_N_CONDITIONS);
}

void sm_set_number_of_conditions(uint8_t number) {
	eeprom_update_byte ((uint8_t *)EE_STATEMACHINE_N_CONDITIONS, number);
}

uint8_t sm_get_number_of_transitions(bool datetime) {
	return (datetime ? eeprom_read_byte ((const void*)EE_STATEMACHINE_N_DT_TRANSITIONS) : eeprom_read_byte ((const void*)EE_STATEMACHINE_N_TRANSITIONS));
}

void sm_set_number_of_transitions(bool datetime, uint8_t number) {
	if(datetime)
		eeprom_update_byte ((uint8_t *)EE_STATEMACHINE_N_DT_TRANSITIONS, number);
	else 
		eeprom_update_byte ((uint8_t *)EE_STATEMACHINE_N_TRANSITIONS, number);
}

void sm_write_condition(uint8_t index, struct condition *cond) {
	eeprom_update_block(cond, (void *)(EE_STATEMACHINE_CONDITIONS + sizeof(struct condition)*index), sizeof(struct condition));
}

void sm_get_condition(uint8_t index, struct condition *cond) {
	eeprom_read_block(cond, (void *)(EE_STATEMACHINE_CONDITIONS + sizeof(struct condition)*index), sizeof(struct condition));
}

void sm_set_id(uint8_t id) {
  eeprom_update_byte((uint8_t *)EE_STATEMACHINE_ID, id);
}

void sm_write_transition(bool datetime, uint8_t index, struct transition *trans) {
	if(datetime)
		eeprom_update_block(trans, (void *)(EE_STATEMACHINE_DATETIME_TRANSITIONS + sizeof(struct transition)*index), sizeof(struct transition));
	else
		eeprom_update_block(trans, (void *)(EE_STATEMACHINE_TRANSITIONS + sizeof(struct transition)*index), sizeof(struct transition));
}

void sm_get_transition(bool datetime, uint8_t index, struct transition *trans) {
	if(datetime)
		eeprom_read_block(trans, (void *)(EE_STATEMACHINE_DATETIME_TRANSITIONS + sizeof(struct transition)*index), sizeof(struct transition));
	else
		eeprom_read_block(trans, (void *)(EE_STATEMACHINE_TRANSITIONS + sizeof(struct transition)*index), sizeof(struct transition));
}

bool sm_write_chunk(uint8_t chunk_id, char* data) {
  /**
   * 1. Check for valid offset: if offset+length > MAX_SIZE abort.
   * 2. Write Block:
	eeprom_update_block(cond, (void *)(EE_STATEMACHINE_CONDITIONS + sizeof(struct condition)*index), sizeof(struct condition));

   */
  //printf("Chunk ID: %d, Chunk offset: %d\r\n", chunk_id, chunk_id*EE_STATEMACHINE_CHUNK_SIZE);
  if(((chunk_id+1) * (EE_STATEMACHINE_CHUNK_SIZE)) 
      > EE_STATEMACHINE_ID_SIZE +
        EE_STATEMACHINE_N_CONDITIONS_SIZE + EE_STATEMACHINE_CONDITIONS_SIZE
      + EE_STATEMACHINE_N_DT_TRANSITIONS_SIZE + EE_STATEMACHINE_DATETIME_TRANSITIONS_SIZE
      + EE_STATEMACHINE_N_TRANSITIONS_SIZE + EE_STATEMACHINE_TRANSITIONS_SIZE) 
  {
    printf("offset too large - not writing to EEPROM.\r\n");
    return false;
  } else {
    //printf("writing to eeprom at %u\r\n", chunk_id*EE_STATEMACHINE_CHUNK_SIZE);
    cli();
		eeprom_update_block(data,
        (void *)(EE_STATEMACHINE_ID + (chunk_id * EE_STATEMACHINE_CHUNK_SIZE)), 
        EE_STATEMACHINE_CHUNK_SIZE
      );
    eeprom_busy_wait();
    sei();
    // The write fails if no function call follows an EEPROM write. This is strangeness.
    _delay_ms(2);
    return true;
  }
    
}
