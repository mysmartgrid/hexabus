#ifndef SEQUENCE_NUMBERS_H_
#define SEQUENCE_NUMBERS_H_

#include <stddef.h>
#include "net/uip.h"
#include "hexabus_types.h"

#define SEQNUM_TABLE_LENGTH 16

void sequence_number_init();
enum hxb_error_code insert_sequence_number(const uip_ipaddr_t* toaddr, uint16_t seqnum);
uint16_t next_sequence_number();


#endif