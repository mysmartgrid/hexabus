#ifndef APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H
#define APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void sm_get_id(char* b);
bool sm_id_equals(const char* b);
bool sm_id_is_zero();

bool sm_write_chunk(uint8_t chunk_id, char* data);

int sm_get_block(uint16_t at, uint8_t size, void* block);
int sm_get_u8(uint16_t at, uint32_t* u);
int sm_get_u16(uint16_t at, uint32_t* u);
int sm_get_u32(uint16_t at, uint32_t* u);
int sm_get_float(uint16_t at, float* f);

#endif /* APPS_STATE_MACHINE_STATE_MACHINE_EEPROM_H */
