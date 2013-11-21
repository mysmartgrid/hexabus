#ifndef SEQUENCE_NUMBERS_H_
#define SEQUENCE_NUMBERS_H_

#include <stddef.h>
#include "net/uip.h"

#define SEQNUM_TABLE_LENGTH 16

struct seqnum_entry {
	uint16_t ip_hash;
	uint16_t seqnum;
};

void init_sequence_number_table();
void insert_sequence_number(const uip_ipaddr_t* toaddr, uint16_t seqnum);
uint16_t next_sequence_number(const uip_ipaddr_t* toaddr);


#endif