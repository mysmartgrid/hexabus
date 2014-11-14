#include <stdlib.h>
#include <string.h>

#include "sequence_numbers.h"
#include "lib/crc16.h"
#include "lib/random.h"
#include "hexabus_types.h"

static uint16_t seqnum_table[SEQNUM_TABLE_LENGTH];

uint8_t hash_ip(const uip_ipaddr_t* target_ip, uint16_t port) {
	unsigned char raw_data[18];
	memcpy(raw_data, target_ip, 16*sizeof(unsigned char));
	memcpy(&raw_data[16], &port, 2*sizeof(unsigned char));
	uint16_t crc = crc16_data(raw_data, 18, 0);
	return (uint8_t)(((crc>>12)^(crc>>8)^(crc>>4)^crc)&0x000f);
}

uint16_t next_sequence_number(const uip_ipaddr_t* target_ip, uint16_t port) {

	uint16_t next = ++seqnum_table[hash_ip(target_ip, port)]==0?
		seqnum_table[hash_ip(target_ip, port)]++:seqnum_table[hash_ip(target_ip, port)];

	return next;
}

uint16_t get_sequence_number(const uip_ipaddr_t* target_ip, uint16_t port) {
	return seqnum_table[hash_ip(target_ip, port)];
}

void sequence_number_init() {
	int i;
	for (i = 0; i < SEQNUM_TABLE_LENGTH; i++)
	{
		do {
			seqnum_table[i] = random_rand();
		} while(seqnum_table[i]==0);
	}
}
