#include <stdlib.h>

#include "sequence_numbers.h"
#include "lib/crc16.h"
#include "lib/random.h"
#include "hexabus_types.h"

static uint16_t seqnum_table[SEQNUM_TABLE_LENGTH];

uint8_t hash_ip(const uip_ipaddr_t* toaddr) {
	uint16_t crc = crc16_data((unsigned char*)toaddr, 16, 0);
	return (uint8_t)(((crc>>12)^(crc>>8)^(crc>>4)^crc)&0x000f);
}

uint16_t next_sequence_number(const uip_ipaddr_t* toaddr) {

	return ++seqnum_table[hash_ip(toaddr)]==0?
		seqnum_table[hash_ip(toaddr)]++:seqnum_table[hash_ip(toaddr)];
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
