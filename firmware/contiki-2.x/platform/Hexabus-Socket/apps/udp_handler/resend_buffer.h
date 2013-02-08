#ifndef RESEND_BUFFER_H_
#define RESEND_BUFFER_H_

#include <stddef.h>
#include "../../../../../../shared/hexabus_packet.h"

#define RESEND_BUFFER_SIZE 3

int write_to_buffer(uint16_t seqnum, void* data, int length);
int read_from_buffer(uint16_t seqnum, void** data, int* length);
int clear_buffer();
int init_buffer();

uint16_t increase_seqnum();
uint16_t current_seqnum();
uint16_t set_seqnum(uint16_t seqnum);
uint8_t seqnum_is_valid();
void set_seqnum_valid();
void set_seqnum_invalid();
int compare_seqnum_greater_than(uint16_t seqnum);
void resend_timeout_handler();

#endif
