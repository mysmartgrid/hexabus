#ifndef SEQUENCE_NUMBER_H_
#define SEQUENCE_NUMBER_H_

#include <stdint.h>
#include "process.h"

uint16_t increase_seqnum();
uint16_t current_seqnum();
uint16_t set_seqnum(uint16_t seqnum);
uint16_t increase_seqnum();
uint16_t current_seqnum();
uint16_t set_seqnum(uint16_t seqnum);
uint8_t seqnum_is_valid();
void set_seqnum_valid();
void set_seqnum_invalid();
int compare_seqnum_greater_than(uint16_t seqnum);
void send_flag_packet(void* data, int len, uint16_t flags);
int parse_seqnum_header(int offset);

#if RAVEN_REVSISION!=HEXABUS_USB
PROCESS_NAME(resend_init_process);
#endif

#endif
