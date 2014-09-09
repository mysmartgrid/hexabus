#include "state_machine_eeprom.h"

#include "nvm.h"
#include "hexabus_statemachine_structs.h"
#include "sm_types.h"

#include <string.h>
#include "uip.h"

void sm_get_id(char* b)
{
	nvm_read_block(sm.id, b, nvm_size(sm.id));
}

bool sm_id_equals(const char* b)
{
	char id[16];

	sm_get_id(id);
	return memcmp(b, id, 16) == 0;
}

bool sm_id_is_zero()
{
	return sm_id_equals("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
}

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

int sm_get_u8(uint16_t at, uint32_t* u)
{
	uint8_t buf;

	if (sm_get_block(at, 1, &buf) < 0)
		return -HSE_OOB_READ;

	*u = buf;
	return 1;
}

int sm_get_u16(uint16_t at, uint32_t* u)
{
	uint16_t buf;

	if (sm_get_block(at, 2, &buf) < 0)
		return -HSE_OOB_READ;

	*u = uip_ntohs(buf);
	return 2;
}

int sm_get_u32(uint16_t at, uint32_t* u)
{
	uint32_t buf;

	if (sm_get_block(at, 4, &buf) < 0)
		return -HSE_OOB_READ;

	*u = uip_ntohl(buf);
	return 4;
}

int sm_get_float(uint16_t at, float* f)
{
	uint32_t u;

	if (sm_get_u32(at, &u) < 0)
		return -HSE_OOB_READ;

	memcpy(f, &u, 4);
	return 4;
}
