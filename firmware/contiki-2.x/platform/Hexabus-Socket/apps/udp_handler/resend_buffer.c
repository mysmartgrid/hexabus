#include "resend_buffer.h"
#include "value_broadcast.h"
#include "contiki.h"

#include <stdint.h>
#include <stddef.h>

struct buffer_entry {
	uint16_t seqnum;
	void* data;
	int length;
};

static struct buffer_entry* resend_buffer[RESEND_BUFFER_SIZE];
static uint16_t sequence_number = 0;
static uint8_t seqnum_valid = 0;

static struct ctimer reset_trigger;

int write_to_buffer(uint16_t seqnum, void* data, int length) {
	struct buffer_entry* new_entry = malloc(sizeof(struct buffer_entry));
	new_entry->seqnum = seqnum;
	new_entry->data = malloc(length);
	new_entry->length = length;
	memcpy(new_entry->data, data, length);

	int i;
	for(i=0;i<RESEND_BUFFER_SIZE;i++) {
		if(resend_buffer[i] == NULL) {
			resend_buffer[i] = new_entry;
			return 0;
		}
	}

	free(resend_buffer[0]->data);
	free(resend_buffer[0]);
	resend_buffer[0] = NULL;

	for(i=1;i<RESEND_BUFFER_SIZE;i++) {
		resend_buffer[i-1] = resend_buffer[i];
	}

	resend_buffer[RESEND_BUFFER_SIZE-1] = new_entry;

	return 0;
}

int read_from_buffer(uint16_t seqnum, void** data, int* length) {
	int i;
	for(i=0;i<RESEND_BUFFER_SIZE;i++) {
		if(resend_buffer[i]->seqnum == seqnum) {
			*data = malloc(resend_buffer[i]->length);
			memcpy(*data, resend_buffer[i]->data, resend_buffer[i]->length);
			*length = resend_buffer[i]->length;
			return 0;
		}
	}
	
	return -1;
}

int clear_buffer() {
	int i;
	for(i=0;i<RESEND_BUFFER_SIZE;i++) {
		free(resend_buffer[i]->data);
		free(resend_buffer[i]);
		resend_buffer[i] = NULL;
	}

	return 0;
}

int init_buffer() {
	int i;
	for(i=0;i<RESEND_BUFFER_SIZE;i++) {
		resend_buffer[i] = NULL;
	}

	return 0;
}

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

void trigger_reset() {
	if(seqnum_is_valid()) {
		broadcast_value(12);
	} else {
		send_seqnum_request();
		ctimer_set(&reset_trigger, CLOCK_SECOND, &trigger_reset, NULL);
	}
}

void resend_timeout_handler() {
	printf("Resend request timed out\r\n");
	set_seqnum_invalid();
	send_seqnum_request();
	ctimer_set(&reset_trigger, CLOCK_SECOND, &trigger_reset, NULL);
}
