#ifndef APPS_NVM_NVM_H_01C7A5F0259D7842
#define APPS_NVM_NVM_H_01C7A5F0259D7842

#include <stddef.h>
#include <stdint.h>

enum {
	EEP_SIZE = 4096
};

struct hxb_sm_nvm_layout {
	uint8_t code[2048];
} __attribute__((packed));

struct hxb_nvm_layout {
	uint8_t dummy;

	/* network configuration */
	uint8_t mac_addr[8];
	uint16_t pan_addr;
	uint16_t pan_id;

	uint8_t bootloader_flag;
	uint16_t bootloader_crc;
	uint8_t first_run;

	uint8_t pgm_checksum[16];

	uint8_t encryption_key[16];

	char domain_name[30];

	uint16_t metering_ref;
	uint16_t metering_cal_load;
	uint8_t metering_cal_flag;

	uint8_t forwarding;

	struct hxb_sm_nvm_layout sm;

	/* use the remaining space for properties (EEP_SIZE - trailer - leader)*/
	uint8_t endpoint_properties[EEP_SIZE - 8 - (85 + sizeof(struct hxb_sm_nvm_layout))];

	uint32_t energy_metering_pulses;
	uint32_t energy_metering_pulses_total;
} __attribute__((packed));

static inline void eeprom_layout_size_static_assert(void)
{
	char bug_on[1 - 2 * !(sizeof(struct hxb_nvm_layout) == EEP_SIZE)];
	(void) bug_on;
}

#define nvm_addr(field) (offsetof(struct hxb_nvm_layout, field))
#define nvm_size(field) (sizeof(((const struct hxb_nvm_layout*) 0)->field))

void nvm_write_block_at(uintptr_t addr, const void* block, size_t size);
#define nvm_write_block(field, block, size) nvm_write_block_at(nvm_addr(field), block, size)

void nvm_read_block_at(uintptr_t addr, void* block, size_t size);
#define nvm_read_block(field, block, size) nvm_read_block_at(nvm_addr(field), block, size)

inline uint8_t nvm_read_u8_at(uintptr_t addr)
{
	uint8_t result;
	nvm_read_block_at(addr, &result, sizeof(result));
	return result;
}
#define nvm_read_u8(field) nvm_read_u8_at(nvm_addr(field))

inline void nvm_write_u8_at(uintptr_t addr, uint8_t val)
{
	nvm_write_block_at(addr, &val, sizeof(val));
}
#define nvm_write_u8(field, val) nvm_write_u8_at(nvm_addr(field), val)

inline uint16_t nvm_read_u16_at(uintptr_t addr)
{
	uint16_t result;
	nvm_read_block_at(addr, &result, sizeof(result));
	return result;
}
#define nvm_read_u16(field) nvm_read_u16_at(nvm_addr(field))

inline void nvm_write_u16_at(uintptr_t addr, uint16_t val)
{
	nvm_write_block_at(addr, &val, sizeof(val));
}
#define nvm_write_u16(field, val) nvm_write_u16_at(nvm_addr(field), val)

#endif
