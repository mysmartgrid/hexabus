#include "state_machine_eeprom.h"

#include "nvm.h"
#include "hexabus_statemachine_structs.h"
#include "sm_types.h"

#include <string.h>
#include "uip.h"

bool sm_write_chunk(uint8_t chunk_id, char* data) {
	// check where the chunk goes: the first chunk goes to the domain_name area of the eeprom
	// chunk 0 may contain a device name (but need not), the other chunks are state machine data
	if (chunk_id == 0) {
		if (!data[0])
			return true;

		nvm_write_block(domain_name, data, nvm_size(domain_name));

		return true;
	} else {
		if (chunk_id * EE_STATEMACHINE_CHUNK_SIZE > sizeof(struct hxb_sm_nvm_layout))
			return false;

		nvm_write_block_at(nvm_addr(sm) + (chunk_id - 1) * EE_STATEMACHINE_CHUNK_SIZE, data, EE_STATEMACHINE_CHUNK_SIZE);

		return true;
	}
}

int sm_get_block(uint16_t at, uint8_t size, void* block)
{
	if (at + size > nvm_size(sm))
		return -HSE_OOB_READ;

	nvm_read_block_at(nvm_addr(sm) + at, block, size);

	return size;
}
