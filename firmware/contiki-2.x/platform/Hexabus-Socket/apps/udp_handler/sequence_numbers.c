#include <stdlib.h>

#include "sequence_numbers.h"
#include "lib/crc16.h"
#include "lib/random.h"

static struct seqnum_entry* seqnum_table[SEQNUM_TABLE_LENGTH];
static uint8_t list_end = 0;

void init_sequence_number_table() {
	int i;
	for(i=0; i<SEQNUM_TABLE_LENGTH; i++) {
		if(seqnum_table[i]!=NULL) {
			free(seqnum_table[i]);
			seqnum_table[i] = NULL;
		}
	}
}

void insert_entry(struct seqnum_entry* new_entry) {
	
	int i;
	for(i=0; i<SEQNUM_TABLE_LENGTH; i++) {
		if(seqnum_table[i]==NULL)
			break;

		if(new_entry->ip_hash == seqnum_table[i]->ip_hash) {
			seqnum_table[i]->seqnum = new_entry->seqnum;
			return;
		}
	}

	free(seqnum_table[list_end]);
	seqnum_table[list_end] = new_entry;
	list_end++;
	if(list_end>=SEQNUM_TABLE_LENGTH)
		list_end=0;
}

uint16_t next_sequence_number(const uip_ipaddr_t* toaddr) {
	struct seqnum_entry* new_entry = NULL;
	uint16_t hash = crc16_data((unsigned char*)toaddr, 16, 0);

	int i;
	for(i=0; i<SEQNUM_TABLE_LENGTH; i++) {
		if(seqnum_table[i]==NULL)
			break;

		if(hash == seqnum_table[i]->ip_hash)
			return seqnum_table[i]->seqnum++;
	}

	new_entry = malloc(sizeof(struct seqnum_entry));
	new_entry->ip_hash = hash;
	new_entry->seqnum = random_rand();

	insert_entry(new_entry);

	return new_entry->seqnum;
}

void insert_sequence_number(const uip_ipaddr_t* toaddr, uint16_t seqnum) {
	struct seqnum_entry* new_entry = NULL;
	new_entry->ip_hash = crc16_data((unsigned char*)toaddr, 16, 0);
	new_entry->seqnum = seqnum;

	insert_entry(new_entry);
}
