#include <stdlib.h>
#include <stdio.h>

#include "sequence_numbers.h"
#include "lib/crc16.h"
#include "lib/random.h"
#include "hexabus_types.h"

static uint16_t *seqnum_table[SEQNUM_TABLE_LENGTH];

uint16_t next_sequence_number(const uip_ipaddr_t* toaddr) {

	uint16_t crc = crc16_data((unsigned char*)toaddr, 16, 0);
	uint8_t hash = (uint8_t)(((crc>>12)^(crc>>8)^(crc>>4)^crc)&0x000f);

	if(seqnum_table[hash] == NULL) {
		seqnum_table[hash] = malloc(sizeof(uint16_t));
		*seqnum_table[hash] = random_rand();
	} else {
		*seqnum_table[hash]++;
	}

	return *seqnum_table[hash];


}

void sequence_number_init() {
	int i;
	for (i = 0; i < SEQNUM_TABLE_LENGTH; i++)
	{
		seqnum_table[i] = NULL;
	}
}
