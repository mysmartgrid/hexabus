#include "resend_buffer.h"
#include "contiki.h"
#include "lib/memb.h"
#include "lib/list.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

struct buffer_entry {
	struct buffer_entry *next;
	uint16_t seqnum;
	void* data;
	int length;
};

MEMB(resend_buffer, struct buffer_entry, RESEND_BUFFER_SIZE);
LIST(resend_buffer_list);

int write_to_buffer(uint16_t seqnum, const void* data, int length) {
	struct buffer_entry* e;
	struct buffer_entry* old;


	while((e=memb_alloc(&resend_buffer))==NULL) {
		old = list_chop(resend_buffer_list);
		free(old->data);
		memb_free(&resend_buffer,old);
	}

	void *sdata = malloc(length);
	memcpy(sdata,data,length);

	e->seqnum = seqnum;
	e->data = sdata;
	e->length = length;

	list_push(resend_buffer_list, e);

	return 0;
}

int read_from_buffer(uint16_t seqnum, void* data, int* length) {

	struct buffer_entry* e = list_head(resend_buffer_list);

	while(e != NULL) {
		if(e->seqnum == seqnum) {
			void *rdata = malloc(e->length);
			memcpy(rdata,e->data,e->length);
			*length = e->length;
			data = rdata;
			return 0;
		}
		e = list_item_next(e);
	}
	return -1;
}

int clear_buffer() {
	struct buffer_entry* e;

	while(list_length(resend_buffer_list)) {
		e = list_pop(resend_buffer_list);
		free(e->data);
		memb_free(&resend_buffer, e);
	}
	return 0;
}

int init_buffer() {
	memb_init(&resend_buffer);
	list_init(resend_buffer_list);
	return 0;
}

