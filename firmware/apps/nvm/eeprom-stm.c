#include "nvm.h"

#include <stm32l1xx.h>
#include <string.h>

extern uint8_t _eep[];

void nvm_write_block_at(uintptr_t addr, const void* block, size_t size)
{
	uintptr_t block_base = ((uintptr_t) _eep) + addr;
	const char* data = block;

	FLASH->PEKEYR = 0x89ABCDEF;
	FLASH->PEKEYR = 0x02030405;
	FLASH->PECR &= ~FLASH_PECR_FTDW;
	while (size) {
		unsigned bytes = size < 4 ? size : 4;
		unsigned misalign = block_base & 3;

		if (misalign) {
			bytes = size < 4 - misalign ? size : 4 - misalign;
			block_base &= ~3;
		}

		uint32_t word = *((volatile uint32_t*) block_base);
		memcpy(((char*) &word) + misalign, data, bytes);
		*((volatile uint32_t*) block_base) = word;

		size -= bytes;
		data += bytes;
		block_base += sizeof(uint32_t);
	}
	FLASH->PECR |= FLASH_PECR_PELOCK;
}

void nvm_read_block_at(uintptr_t addr, void* block, size_t size)
{
	memcpy(block, _eep + addr, size);
}
