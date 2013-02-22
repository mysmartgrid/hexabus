#include "sequence_number.h"

static uint16_t sequence_number = 0;
static uint8_t seqnum_valid = 0;

uint16_t increase_seqnum() {
	sequence_number++;
	return sequence_number;
}

uint16_t set_seqnum(uint16_t seqnum) {
	sequence_number = seqnum;
	return sequence_number;
}

uint16_t current_seqnum() {
	return sequence_number;
}

int compare_seqnum_greater_than(uint16_t seqnum) {
	//cf. RFC 1982
	if(((int16_t)seqnum - (int16_t)sequence_number)>0)
		return 1;
	else
		return 0;
}

uint8_t seqnum_is_valid() {
	return seqnum_valid;
}

void set_seqnum_valid() {
	seqnum_valid = 1;
}

void set_seqnum_invalid() {
	seqnum_valid = 0;
}
