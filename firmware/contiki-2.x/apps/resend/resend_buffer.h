#ifndef RESEND_BUFFER_H_
#define RESEND_BUFFER_H_

#define RESEND_BUFFER_SIZE 3

#include <stdint.h>

int write_to_buffer(uint16_t seqnum, const void* data, int length);
int read_from_buffer(uint16_t seqnum, void* data, int* length);
int clear_buffer();
int init_buffer();

#endif
