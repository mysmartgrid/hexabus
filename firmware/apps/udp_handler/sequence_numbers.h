#ifndef SEQUENCE_NUMBERS_H_
#define SEQUENCE_NUMBERS_H_

#include <stddef.h>
#include "net/uip.h"
#include "hexabus_types.h"

#define SEQNUM_TABLE_LENGTH 16

uint8_t hash_ip(const uip_ipaddr_t* toaddr);
void sequence_number_init();
uint16_t next_sequence_number(const uip_ipaddr_t* toaddr);


#endif