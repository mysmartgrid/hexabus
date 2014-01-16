#include "reliability.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hexabus_config.h"
#include "process.h"
#include "net/packetqueue.h"
#include "sequence_numbers.h"
#include "udp_handler.h"

#define MAX(a,b) (((a)>(b))?(a):(b)) //TODO hmmm...

enum hxb_send_state_t {
	SINIT		= 0,
	SREADY		= 1,
	SWAIT_ACK	= 2,
	SFAILED		= 3,
};

enum hxb_recv_state_t {
	RINIT 		= 0,
	RREADY 		= 1,
	RFAILED 	= 2,
};

static enum hxb_send_state_t send_state = SINIT;
static enum hxb_recv_state_t recv_state = RINIT;
static uint8_t fail;
static bool ack;
static uint8_t retrans;
static uint16_t want_ack_for;
static struct etimer timeout_timer;
static struct hxb_queue_packet R;
static bool recved_packet;
static uint16_t rseq_num;

PROCESS(reliability_send_process, "Reliability sending state machine");
AUTOSTART_PROCESSES(&reliability_send_process);

PACKETQUEUE(send_queue, SEND_QUEUE_SIZE);

enum hxb_error_code packet_generator(union hxb_packet_any* buffer, void* data) {
	
	if(want_ack_for == 0) {
		want_ack_for = next_sequence_number(&((struct hxb_queue_packet*)data)->ip);
	}

	memcpy(buffer, &((struct hxb_queue_packet*)data)->packet, sizeof(union hxb_packet_any));
	buffer->header.sequence_number = want_ack_for;

	return HXB_ERR_SUCCESS;
}

enum hxb_error_code send_packet(struct hxb_queue_packet* data) {
	return udp_handler_send_generated(&((struct hxb_queue_packet*)data)->ip, ((struct hxb_queue_packet*)data)->port, &packet_generator, (void*)data);
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
		case SINIT:
			fail = 0;
			send_state = SREADY;
			break;
		case SREADY:
			if (packetqueue_len(&send_queue) > 0) {
				if(!send_packet((struct hxb_queue_packet *)packetqueue_first(&send_queue))) {
					retrans = 0;
					ack = false;
					send_state = SWAIT_ACK;
					etimer_set(&timeout_timer, RETRANS_TIMEOUT);
				}
			} else {
				//TODO something went wrong
			}

			break;
		case SWAIT_ACK:
			if(ack) {
				want_ack_for = 0;
				free(packetqueue_first(&send_queue));
				packetqueue_dequeue(&send_queue);
				send_state = SREADY;
			} else if(etimer_expired(&timeout_timer)) {
				if(retrans++ < RETRANS_LIMIT) {
					send_packet((struct hxb_queue_packet *)packetqueue_first(&send_queue));
					etimer_set(&timeout_timer, RETRANS_TIMEOUT);
				} else {
					fail = 1;
					send_state = SFAILED;
				}
			}
			break;
		case SFAILED:
			if(fail == 2) {
				//TODO recover
			}
			break;
	}
}

enum hxb_error_code receive_packet(struct hxb_queue_packet* packet) {
	if(memcpy(&R, packet, sizeof(struct hxb_queue_packet))) {
		return HXB_ERR_OUT_OF_MEMORY;
	}

	recved_packet = true;

	return HXB_ERR_SUCCESS;
}

bool allows_implicit_ack(union hxb_packet_any* packet) {
	switch(packet->header.type) {
		case HXB_PTYPE_QUERY:
		case HXB_PTYPE_EPQUERY:
		case HXB_PTYPE_WRITE:
			return true;
			break;
		default:
			return false;
	}
}

bool is_ack_for(union hxb_packet_any* packet, uint16_t seq_num) {
	switch(packet->header.type) {
		case HXB_PTYPE_REPORT:
		case HXB_PTYPE_EPREPORT:
		case HXB_PTYPE_ACK:
			return (seq_num == packet->header.sequence_number);
			break;
		default:
			return false;
	}
}

static void run_recv_state_machine() {
	switch(recv_state) {
		case RINIT:
			rseq_num = 0;
			recv_state = RREADY;
			break;
		case RREADY:
			if(fail) {
				recv_state = RFAILED;
			} else if(recved_packet){
				if((R.packet.header.flags & HXB_FLAG_WANT_ACK) && !allows_implicit_ack(&(R.packet))) { //TODO only use flag?
					udp_handler_send_ack(&(R.ip), R.port, R.packet.header.sequence_number);

					if(R.packet.header.sequence_number <= rseq_num) {
						//TODO handle reordering here
						recv_state = RREADY;
						break;
					}
				}

				rseq_num = MAX(rseq_num, R.packet.header.sequence_number);

				if(want_ack_for != 0 && is_ack_for(&(R.packet), want_ack_for)) {
					ack = true;
					recv_state = RREADY;
				} else {
					//TODO packet to upper layer
					recv_state = RREADY;
				}
			}
			break;
		case RFAILED:
			if(false) { //TODO if correct packet from master
				fail = 0;
				recv_state = RREADY;
			}
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