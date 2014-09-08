#include "nvm.h"

#include <avr/eeprom.h>

void nvm_write_block_at(uintptr_t addr, const void* block, size_t size)
{
	eeprom_busy_wait();
	eeprom_write_block(block, (void*) addr, size);
	eeprom_busy_wait();
}

void nvm_read_block_at(uintptr_t addr, void* block, size_t size)
{
	eeprom_busy_wait();
	eeprom_read_block(block, (void*) addr, size);
}
