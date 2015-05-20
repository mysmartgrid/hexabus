#include "reliability.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hexabus_config.h"
#include "process.h"
#include "net/packetqueue.h"
#include "sequence_numbers.h"
#include "udp_handler.h"
#include "state_machine.h"
#include "endpoints.h"

#define LOG_LEVEL UDP_HANDLER_DEBUG
#include "syslog.h"

#define MAX(a,b) (seqnumIsLessEqual((a),(b))?(b):(a))

extern uip_ipaddr_t udp_master_addr;

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

PROCESS(reliability_send_process, "Reliability sending state machine");
AUTOSTART_PROCESSES(&reliability_send_process);

PACKETQUEUE(send_queue, SEND_QUEUE_SIZE);

process_event_t reliability_event;

struct reliability_state {
	enum hxb_send_state_t send_state;
	enum hxb_recv_state_t recv_state;
	uint8_t fail;
	bool ack;
	uint8_t retrans;
	uint16_t want_ack_for;
	bool isUlAck;
	uint16_t ul_ack_state;
	struct etimer timeout_timer;
	bool recved_packet;
	bool do_reset;
	uint16_t rseq_num;
	struct hxb_queue_packet* P;
};

struct reliability_state rstates[16];

static uint8_t fail;
static struct hxb_queue_packet* R;

bool seqnumIsLessEqual(uint16_t first, uint16_t second) {
	int16_t distance = (int16_t)(first - second);
	return distance <= 0;
}

void send_packet(struct hxb_queue_packet* data) {
	uint8_t hash = hash_ip(&(data->ip), data->port);

	data->packet.header.flags |= HXB_FLAG_RELIABLE;

	if(rstates[hash].do_reset) {
		syslog(LOG_DEBUG, "Sending connection reset to " LOG_6ADDR_FMT , LOG_6ADDR_VAL(data->ip));
		data->packet.header.flags |= HXB_FLAG_CONN_RESET;
	}

	if(data->packet.header.sequence_number == 0) {
		rstates[hash].want_ack_for = data->packet.header.sequence_number = next_sequence_number(&(data->ip), data->port);
	}

	syslog(LOG_DEBUG, "Sending packet %u", rstates[hash].want_ack_for);

	do_udp_send(&(data->ip), data->port, &(data->packet));
}

enum hxb_error_code enqueue_packet(const uip_ipaddr_t* toaddr, uint16_t toport, union hxb_packet_any* packet) {

	struct hxb_queue_packet* queue_entry = malloc(sizeof(struct hxb_queue_packet));

	if(queue_entry == NULL)
		return HXB_ERR_OUT_OF_MEMORY;

	queue_entry->port = toport;
	memcpy(&(queue_entry->ip), toaddr, sizeof(uip_ipaddr_t));

	if(packet != NULL) {
		memcpy(&(queue_entry->packet), packet, sizeof(union hxb_packet_any));

		if(!packetqueue_enqueue_packetbuf(&send_queue, 0, (void*) queue_entry)) {
			return HXB_ERR_OUT_OF_MEMORY;
		}
	} else {
		return HXB_ERR_NO_VALUE;
	}

	return HXB_ERR_SUCCESS;
}

void enqueue_sm_reset() {
	union hxb_packet_any packet_sm_control = {
		.p_u8.type = HXB_PTYPE_WRITE,
		.p_u8.flags = HXB_FLAG_WANT_ACK,
		.p_u8.eid = EP_SM_CONTROL,
		.p_u8.datatype = HXB_DTYPE_UINT8,
		.p_u8.value = STM_STATE_RUNNING
	};

#if VALUE_BROADCAST_RELIABLE
	enqueue_packet(&udp_master_addr, HXB_PORT, (union hxb_packet_any*) &packet_sm_control);
#else
	do_udp_send(&udp_master_addr, HXB_PORT, (union hxb_packet_any*) &packet_sm_control);
#endif
}

enum hxb_error_code ul_ack_received(const uip_ipaddr_t* toaddr, uint16_t port, uint16_t seq_num) {
	uint8_t rs = hash_ip(toaddr, port);


	if(seq_num == rstates[rs].want_ack_for) {
		syslog(LOG_DEBUG, "Received UL_ACK for %u", seq_num);
		rstates[rs].ack = true;
		rstates[rs].recv_state = RREADY;

	}

	return HXB_ERR_SUCCESS;
}

enum hxb_error_code ul_send_ack(const uip_ipaddr_t* toaddr, uint16_t port, uint16_t seq_num) {
	uint8_t rs = hash_ip(toaddr, port);

	syslog(LOG_DEBUG, "Sending UL_ACK for %u", seq_num);

	udp_handler_send_ack(toaddr, port, seq_num);

	rstates[rs].ul_ack_state = seq_num;

	return HXB_ERR_SUCCESS;
}

bool allows_implicit_ack(union hxb_packet_any* packet) {
	switch(packet->header.type) {
		case HXB_PTYPE_QUERY:
		case HXB_PTYPE_EPQUERY:
		case HXB_PTYPE_WRITE:
		case HXB_PTYPE_ACK:
		case HXB_PTYPE_EP_PROP_QUERY:
		case HXB_PTYPE_EP_PROP_WRITE:
			return true;
			break;
		default:
			return false;
	}
}

bool is_ack_for(union hxb_packet_any* packet, uint16_t seq_num) {
	switch(packet->header.type) {
		case HXB_PTYPE_REPORT:
			switch ((enum hxb_datatype) packet->value_header.datatype) {
				case HXB_DTYPE_BOOL:
				case HXB_DTYPE_UINT8:
					return uip_ntohs(packet->p_report_u8.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_UINT16:
					return uip_ntohs(packet->p_report_u16.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_UINT32:
					return uip_ntohs(packet->p_report_u32.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_UINT64:
					return uip_ntohs(packet->p_report_u64.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_SINT8:
					return uip_ntohs(packet->p_report_s8.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_SINT16:
					return uip_ntohs(packet->p_report_s16.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_SINT32:
					return uip_ntohs(packet->p_report_s32.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_SINT64:
					return uip_ntohs(packet->p_report_s64.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_FLOAT:
					return uip_ntohs(packet->p_report_float.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_128STRING:
					return uip_ntohs(packet->p_report_128string.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_65BYTES:
					return uip_ntohs(packet->p_report_65bytes.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_16BYTES:
					return uip_ntohs(packet->p_report_16bytes.cause_sequence_number) == seq_num;
					break;

				case HXB_DTYPE_UNDEFINED:
				default:
					syslog(LOG_ERR, "Packet of unknown datatype.");
					return false;
			}
			break;
		case HXB_PTYPE_EPREPORT:
			return uip_ntohs(packet->p_report_u32.cause_sequence_number) == seq_num;
			break;
		case HXB_PTYPE_ACK:
			return uip_ntohs(packet->p_ack.cause_sequence_number) == seq_num;
			break;
		default:
			return false;
	}
}

static void run_send_state_machine(uint8_t rs) {
	switch(rstates[rs].send_state) {
		case SINIT:
			rstates[rs].want_ack_for = 0;
			rstates[rs].retrans = 0;
			rstates[rs].send_state = SREADY;
			rstates[rs].P = NULL;
			rstates[rs].do_reset = true;
			break;
		case SREADY:
			if (packetqueue_len(&send_queue) > 0) {
				if(hash_ip(&((struct hxb_queue_packet *)(packetqueue_first(&send_queue))->ptr)->ip, ((struct hxb_queue_packet *)(packetqueue_first(&send_queue))->ptr)->port) == rs) {
					rstates[rs].P = (struct hxb_queue_packet *)(packetqueue_first(&send_queue)->ptr);
					packetqueue_dequeue(&send_queue);

					send_packet(rstates[rs].P);
					rstates[rs].retrans = 0;
					rstates[rs].ack = false;
					rstates[rs].isUlAck = (rstates[rs].P->packet.header.flags)&HXB_FLAG_WANT_UL_ACK;
					rstates[rs].send_state = SWAIT_ACK;
					etimer_set(&(rstates[rs].timeout_timer), RETRANS_TIMEOUT);
				}
			}
			break;
		case SWAIT_ACK:
			if(rstates[rs].ack) {
				rstates[rs].want_ack_for = 0;
				rstates[rs].do_reset = false;
				free(rstates[rs].P);
				rstates[rs].P = NULL;
				rstates[rs].send_state = SREADY;
			} else if(etimer_expired(&(rstates[rs].timeout_timer))) {
				if((rstates[rs].retrans)++ <= RETRANS_LIMIT) {
					syslog(LOG_INFO, "Retransmitting %u (%u tries)", rstates[rs].want_ack_for , rstates[rs].retrans-1);
					send_packet((struct hxb_queue_packet *)rstates[rs].P);
					etimer_set(&(rstates[rs].timeout_timer), RETRANS_TIMEOUT*rstates[rs].retrans);
				} else {
					fail = 1;
					rstates[rs].send_state = SFAILED;
					syslog(LOG_WARN, "Reached maximum retransmissions, attempting recovery...");
				}
			}
			break;
		case SFAILED:
			if(fail == 2) {
				syslog(LOG_WARN, "Recovery...");
				reset_reliability_layer();
				fail = 0;
				rstates[rs].send_state = SINIT;
			}
			break;
	}
}

static void run_recv_state_machine(uint8_t rs) {
	switch(rstates[rs].recv_state) {
		case RINIT:
			rstates[rs].rseq_num = 0;
			rstates[rs].ack = false;
			rstates[rs].recved_packet = false;
			rstates[rs].recv_state = RREADY;
			rstates[rs].ul_ack_state = 0;
			break;
		case RREADY:
			if(fail!=0) {
				rstates[rs].recv_state = RFAILED;
			} else if(rstates[rs].recved_packet){
				if(R->packet.header.flags & HXB_FLAG_CONN_RESET) {
					syslog(LOG_DEBUG, "Received reset from " LOG_6ADDR_FMT , LOG_6ADDR_VAL(R->ip));
					rstates[rs].rseq_num=0;
				}

				if(((R->packet.header.flags & HXB_FLAG_WANT_ACK) && !allows_implicit_ack(&(R->packet))) ||
						((R->packet.header.flags & HXB_FLAG_WANT_UL_ACK) && rstates[rs].ul_ack_state==uip_ntohs(R->packet.header.sequence_number))) {
						syslog(LOG_DEBUG, "Sending ack for %u.", uip_ntohs(R->packet.header.sequence_number));
					udp_handler_send_ack(&(R->ip), R->port, uip_ntohs(R->packet.header.sequence_number));
				}

				if((seqnumIsLessEqual(uip_ntohs(R->packet.header.sequence_number), rstates[rs].rseq_num))&&
						(rstates[rs].rseq_num!=0)&&
						!allows_implicit_ack(&(R->packet))&&
						R->packet.header.flags & HXB_FLAG_RELIABLE) {
					syslog(LOG_INFO, "Dropped duplicate (Got: %u, Current: %u).", uip_ntohs(R->packet.header.sequence_number), rstates[rs].rseq_num);
					rstates[rs].recv_state = RREADY;
					break;
				}

				if(R->packet.header.flags & HXB_FLAG_RELIABLE) {
					if(rstates[rs].rseq_num == 0)
						rstates[rs].rseq_num = uip_ntohs(R->packet.header.sequence_number);
					else
						rstates[rs].rseq_num = MAX(rstates[rs].rseq_num, uip_ntohs(R->packet.header.sequence_number));
					syslog(LOG_DEBUG, "Setting seqnum %u", rstates[rs].rseq_num);
				}

				if(rstates[rs].want_ack_for != 0 && !rstates[rs].isUlAck && is_ack_for(&(R->packet), rstates[rs].want_ack_for)) {
					syslog(LOG_DEBUG, "Received ACK for %u", rstates[rs].want_ack_for);
					rstates[rs].ack = true;
					rstates[rs].recv_state = RREADY;
				}
				udp_handler_handle_incoming(R);
			}
			break;
		case RFAILED:
			if(fail == 0) {
				rstates[rs].recv_state = RINIT ;
			} else { //if(uip_ipaddr_cmp(&(R->ip), &udp_master_addr)) {
				fail = 2;
			}
			break;
	}
}

enum hxb_error_code receive_packet(struct hxb_queue_packet* packet) {

	uint8_t hash = hash_ip(&(packet->ip), packet->port);

	syslog(LOG_DEBUG, "Received packet %u from " LOG_6ADDR_FMT , uip_ntohs(packet->packet.header.sequence_number), LOG_6ADDR_VAL(packet->ip));

	R = packet;
	rstates[hash].recved_packet = true;
	run_recv_state_machine(hash);
	rstates[hash].recved_packet = false;

	if(fail)
		return HXB_ERR_INTERNAL;

	return HXB_ERR_SUCCESS;
}

void init_reliability_layer() {
	for (uint8_t i=0; i<16; ++i) {
		rstates[i].recv_state = RINIT;
		rstates[i].send_state = SINIT;
	}
	packetqueue_init(&send_queue);
	enqueue_sm_reset();
	fail = 0;
}

void reset_reliability_layer() {
	syslog(LOG_DEBUG, "Cleaning up...");
	while(packetqueue_len(&send_queue) > 0) {
		free(packetqueue_first(&send_queue)->ptr);
		packetqueue_dequeue(&send_queue);
	}
	for (uint8_t i=0; i<16; ++i)
	{
		free(rstates[i].P);
	}

	init_reliability_layer();
}

PROCESS_THREAD(reliability_send_process, ev, data)
{
	PROCESS_EXITHANDLER(goto exit);

	PROCESS_BEGIN();

	reliability_event = process_alloc_event();

	init_reliability_layer();

	while (1) {
		PROCESS_WAIT_EVENT();
		if (ev == udp_handler_event && *(udp_handler_event_t*) data == UDP_HANDLER_UP) {
			reset_reliability_layer();
		}

		for (uint8_t i=0; i<16; ++i)
		{
			run_send_state_machine(i);
			run_recv_state_machine(i);
		}
		PROCESS_PAUSE();
		process_post(&reliability_send_process, reliability_event, NULL);
	}

exit: ;

	PROCESS_END();
}