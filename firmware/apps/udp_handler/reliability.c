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

#define LOG_LEVEL UDP_HANDLER_DEBUG
#include "syslog.h"

#define MAX(a,b) (seqnumIsLessEqual((a),(b))?(b):(a))

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
	uint16_t rseq_num;
	struct hxb_queue_packet* P;
	//TODO stats
	uint32_t reset_cnt;
	uint32_t retrans_cnt;
	uint32_t drop_cnt;
	//
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

	if(data->packet.header.sequence_number == 0) {
		rstates[hash].want_ack_for = data->packet.header.sequence_number = next_sequence_number(&(data->ip), data->port);
	}

	do_udp_send(&(data->ip), data->port, &(data->packet));
}

enum hxb_error_code enqueue_packet(const uip_ipaddr_t* toaddr, uint16_t toport, union hxb_packet_any* packet) {

	struct hxb_queue_packet* queue_entry = malloc(sizeof(struct hxb_queue_packet));

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

enum hxb_error_code ul_ack_received(const uip_ipaddr_t* toaddr, uint16_t port, uint16_t seq_num) {
	uint8_t rs = hash_ip(toaddr, port);

	if(seq_num == rstates[rs].want_ack_for) {
		rstates[rs].ack = true;
		rstates[rs].recv_state = RREADY;
	}

	return HXB_ERR_SUCCESS;
}

enum hxb_error_code ul_send_ack(const uip_ipaddr_t* toaddr, uint16_t port, uint16_t seq_num) {
	uint8_t rs = hash_ip(toaddr, port);

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
					return packet->p_u8_re.cause_sequence_number == seq_num;
					break;

				case HXB_DTYPE_UINT32:
					return packet->p_u32_re.cause_sequence_number == seq_num;
					break;

				case HXB_DTYPE_FLOAT:
					return packet->p_float_re.cause_sequence_number == seq_num;
					break;

				case HXB_DTYPE_128STRING:
					return packet->p_128string_re.cause_sequence_number == seq_num;
					break;

				case HXB_DTYPE_65BYTES:
					return packet->p_65bytes_re.cause_sequence_number == seq_num;
					break;

				case HXB_DTYPE_16BYTES:
					return packet->p_16bytes_re.cause_sequence_number == seq_num;
					break;

				case HXB_DTYPE_DATETIME:
				case HXB_DTYPE_TIMESTAMP:
					//TODO should not be used anymore
				case HXB_DTYPE_UNDEFINED:
				default:
					syslog(LOG_ERR, "Packet of unknown datatype.");
					return false;
			}
			break;
		case HXB_PTYPE_EPREPORT:
			return (seq_num == packet->p_u32_re.cause_sequence_number);
			break;
		case HXB_PTYPE_ACK:
			return (seq_num == packet->p_ack.cause_sequence_number);
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
			break;
		case SREADY:
			if (packetqueue_len(&send_queue) > 0) {
				if(hash_ip(&((struct hxb_queue_packet *)(packetqueue_first(&send_queue))->ptr)->ip, ((struct hxb_queue_packet *)(packetqueue_first(&send_queue))->ptr)->port) == rs) {
					rstates[rs].P = (struct hxb_queue_packet *)(packetqueue_first(&send_queue)->ptr);
					packetqueue_dequeue(&send_queue);

					syslog(LOG_DEBUG, "Sending reliable packet.");
					send_packet((struct hxb_queue_packet *)rstates[rs].P);
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
				free(rstates[rs].P);
				rstates[rs].P = NULL;
				rstates[rs].send_state = SREADY;
			} else if(etimer_expired(&(rstates[rs].timeout_timer))) {
				if((rstates[rs].retrans)++ <= RETRANS_LIMIT) {
					syslog(LOG_INFO, "Retransmit %u (waiting %u)", rstates[rs].retrans-1, rstates[rs].want_ack_for );
					rstates[rs].retrans_cnt++;
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
				syslog(LOG_DEBUG, "Conn: %u Retrans: %lu Drop: %lu Reset: %lu", rs, rstates[rs].retrans_cnt, rstates[rs].drop_cnt, rstates[rs].reset_cnt);
				syslog(LOG_WARN, "Resetting...");
				rstates[rs].reset_cnt++;
				process_post(PROCESS_BROADCAST, udp_handler_event, UDP_HANDLER_UP);
				sm_restart();
				fail = 0;
				reset_reliability_layer();
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
				syslog(LOG_DEBUG, "Processing packet. (UL: %u)", rstates[rs].ul_ack_state);
				if(((R->packet.header.flags & HXB_FLAG_WANT_ACK) && !allows_implicit_ack(&(R->packet))) ||
						((R->packet.header.flags & HXB_FLAG_WANT_UL_ACK) && rstates[rs].ul_ack_state==uip_ntohs(R->packet.header.sequence_number))) {
						syslog(LOG_DEBUG, "Sending ack.");
					udp_handler_send_ack(&(R->ip), R->port, uip_ntohs(R->packet.header.sequence_number));
				}

				if((seqnumIsLessEqual(uip_ntohs(R->packet.header.sequence_number), rstates[rs].rseq_num))&&
						(rstates[rs].rseq_num!=0)&&
						!allows_implicit_ack(&(R->packet))&&
						R->packet.header.flags & HXB_FLAG_RELIABLE) {
					syslog(LOG_INFO, "Dropped duplicate (Got: %u, Current: %u).", uip_ntohs(R->packet.header.sequence_number), rstates[rs].rseq_num);
					rstates[rs].drop_cnt++;
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
					syslog(LOG_DEBUG, "Received ACK");
					rstates[rs].ack = true;
					rstates[rs].recv_state = RREADY;
				}
				udp_handler_handle_incoming(R);
				syslog(LOG_DEBUG, "Conn: %u Retrans: %lu Drop: %lu Reset: %lu", rs, rstates[rs].retrans_cnt, rstates[rs].drop_cnt, rstates[rs].reset_cnt);
			}
			break;
		case RFAILED:
			//if(uip_ipaddr_cmp(&(R->ip), &udp_master_addr)) {
				fail = 2;
				rstates[rs].recv_state = RINIT;
			//}
			break;
	}
}

enum hxb_error_code receive_packet(struct hxb_queue_packet* packet) {

	uint8_t hash = hash_ip(&(packet->ip), packet->port);

	syslog(LOG_DEBUG, "Received packet %u from %u.", uip_ntohs(packet->packet.header.sequence_number), hash);

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
	fail = 0;
}

void reset_reliability_layer() {
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

	init_reliability_layer();

	for (uint8_t i=0; i<16; ++i)
	{
		rstates[i].retrans_cnt = rstates[i].reset_cnt = rstates[i].drop_cnt = 0;
	}

	while (1) {
		if (ev == udp_handler_event && *(udp_handler_event_t*) data == UDP_HANDLER_UP) {
			reset_reliability_layer();
		}

		for (uint8_t i=0; i<16; ++i)
		{
			run_send_state_machine(i);
			run_recv_state_machine(i);
		}
		PROCESS_PAUSE();
	}

exit: ;

	PROCESS_END();
}