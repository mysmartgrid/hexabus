#ifndef APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H
#define APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool sm_write_chunk(uint8_t chunk_id, char* data);

int sm_get_block(uint16_t at, uint8_t size, void* block);

#endif /* APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H */
