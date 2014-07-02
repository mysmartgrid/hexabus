#include "state_machine_eeprom.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include "net/uip.h"

#include "../../../../../shared/hexabus_statemachine_structs.h"
#include "sm_types.h"

void sm_get_id(char* b)
{
	eeprom_read_block(b, eep_addr(sm.id), 16);
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

		cli();
		eeprom_update_block(data, eep_addr(domain_name), eep_size(domain_name));
		eeprom_busy_wait();
		sei();

		return true;
	} else {
		if (chunk_id * EE_STATEMACHINE_CHUNK_SIZE > sizeof(struct hxb_sm_eeprom_layout))
			return false;

		chunk_id -= 1;

		cli();
		eeprom_update_block(data, eep_addr(sm.code) + chunk_id * EE_STATEMACHINE_CHUNK_SIZE, EE_STATEMACHINE_CHUNK_SIZE);
		eeprom_busy_wait();
		sei();

		return true;
	}
}

int sm_get_block(uint16_t at, uint8_t size, void* block)
{
	if (at + size > eep_size(sm.code))
		return -HSE_OOB_READ;

	eeprom_read_block(block, eep_addr(sm.code) + at, size);

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
