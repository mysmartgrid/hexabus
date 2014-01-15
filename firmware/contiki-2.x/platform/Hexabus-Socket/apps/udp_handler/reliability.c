#include "reliability.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hexabus_config.h"
#include "process.h"
#include "net/packetqueue.h"
#include "sequence_numbers.h"
#include "udp_handler.h"

enum hxb_send_state_t {
	INIT		= 0,
	READY		= 1,
	WAIT_ACK	= 2,
	FAILED		= 3,
};

enum hxb_recv_state_t {
	INIT 		= 0,
	READY 		= 1,
	FAILED 		= 2,
};

static enum hxb_send_state_t send_state = INIT;
static enum hxb_recv_state_t recv_state = INIT;
static uint8_t fail;
static bool ack;
static uint8_t retrans;
static uint16_t want_ack_for;
static struct etimer timeout_timer;

PROCESS(reliability_send_process, "Reliability sending state machine");
AUTOSTART_PROCESSES(&reliability_send_process);

PACKETQUEUE(send_queue, SEND_QUEUE_SIZE);

enum hxb_error_code packet_generator(union hxb_packet_any* buffer, void* data) {
	
	if(want_ack_for == 0) {
		want_ack_for = next_sequence_number(&((struct hxb_queue_packet*)data)->to_ip);
	}

	memcpy(buffer, &((struct hxb_queue_packet*)data)->packet, sizeof(union hxb_packet_any));
	buffer->header.sequence_number = want_ack_for;

	return HXB_ERR_SUCCESS;
}

enum hxb_error_code send_packet(struct hxb_queue_packet* data) {
	return udp_handler_send_generated(&((struct hxb_queue_packet*)data)->to_ip, ((struct hxb_queue_packet*)data)->to_port, &packet_generator, (void*)data);
}

void process_ack(uint16_t sequence_number) {
	if (sequence_number == want_ack_for) {
		ack = true;
	}
}

enum hxb_error_code enqueue_packet(struct hxb_queue_packet* packet) {

	struct hxb_queue_packet* queue_entry = malloc(sizeof(struct hxb_queue_packet));

	if(packet != NULL) {
		memcpy(queue_entry, packet, sizeof(struct hxb_queue_packet));

		if(packetqueue_enqueue_packetbuf(&send_queue, 0, (void*) queue_entry)) {
			return HXB_ERR_OUT_OF_MEMORY;
		}
	} else {
		return HXB_ERR_NO_VALUE;
	}

	return HXB_ERR_SUCCESS;
}

static void run_send_state_machine() {
	//TODO: proper error handling

	switch(send_state) {
		case INIT:
			fail = 0;
			send_state = READY;
			break;
		case READY:
			if (packetqueue_len(&send_queue) > 0) {
				if(!send_packet((struct hxb_queue_packet *)packetqueue_first(&send_queue))) {
					retrans = 0;
					ack = false;
					send_state = WAIT_ACK;
					etimer_set(&timeout_timer, RETRANS_TIMEOUT);
				}
			} else {
				//TODO something went wrong
			}

			break;
		case WAIT_ACK:
			if(ack) {
				want_ack_for = 0;
				free(packetqueue_first(&send_queue));
				packetqueue_dequeue(&send_queue);
				send_state = READY;
			} else if(etimer_expired(&timeout_timer)) {
				if(retrans++ < RETRANS_LIMIT) {
					send_packet((struct hxb_queue_packet *)packetqueue_first(&send_queue));
					etimer_set(&timeout_timer, RETRANS_TIMEOUT);
				} else {
					fail = 1;
					send_state = FAILED;
				}
			}
			break;
		case FAILED:
			if(fail == 2) {
				//TODO recover
			}
			break;
	}
}

static void run_recv_state_machine() {
	switch(recv_state) {
		case INIT:

			break;
		case READY:

			break;
		case FAILED:

			break;
	}
}

PROCESS_THREAD(reliability_send_process, ev, data)
{
	PROCESS_EXITHANDLER(goto exit);

	PROCESS_BEGIN();

	packetqueue_init(&send_queue);

	while (1) {
		run_send_state_machine();
		run_recv_state_machine();
		//etimer_set(&button_timer, BUTTON_DEBOUNCE_TICKS);
		PROCESS_PAUSE();
	}

exit: ;

	PROCESS_END();
}