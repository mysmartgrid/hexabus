#include "resend_buffer.h"
#include "contiki.h"

#include <stdint.h>
#include <stddef.h>

struct buffer_entry {
	uint16_t seqnum;
	void* data;
	int length;
};

static struct buffer_entry* resend_buffer[RESEND_BUFFER_SIZE];

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

