#ifndef RESEND_BUFFER_H_
#define RESEND_BUFFER_H_

#include <stddef.h>
#include "../../../../../../shared/hexabus_packet.h"

#define RESEND_BUFFER_SIZE 3

int write_to_buffer(uint32_t seqnum, void* data, int length);
int read_from_buffer(uint32_t seqnum, void** data, int* length);
int clear_buffer();
int init_buffer();

uint32_t increase_seqnum();
uint32_t current_seqnum();
uint32_t set_seqnum(uint32_t seqnum);
int compare_seqnum_less_than(uint32_t seqnum);

#endif
