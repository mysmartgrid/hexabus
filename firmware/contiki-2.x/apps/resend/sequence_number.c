#include "sequence_number.h"
#include "resend_buffer.h"
#include "net/uip-udp-packet.h"
#include "../../../../shared/hexabus_definitions.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern uint8_t uip_ext_len;

static uint16_t sequence_number = 0;
static uint8_t seqnum_valid = 0;
static uint8_t missing_packets = 0;
static struct ctimer request_timeout_timer;
static struct uip_udp_conn *udpconn = NULL;

#define REQUEST_TINMEOUT CLOCK_SECOND*2 
#define UDP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_SEQNUM_BUF ((struct uip_ext_hdr_opt_exp_hxb_seqnum *)&uip_buf[uip_l2_l3_hdr_len+offset])

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

int compare_seqnum_less_than(uint16_t seqnum) {
//cf. RFC 1982
if(((int16_t)seqnum - (int16_t)sequence_number)>0)
	return 1;
else
	return 0;
}

uint8_t seqnum_is_valid() {
	if(UIP_HXB_SEQNUM_MASTER) {
		return 1;
	} else {
		return seqnum_valid;
	}
}

void set_seqnum_valid() {
	seqnum_valid = 1;
}

void set_seqnum_invalid() {
	seqnum_valid = 0;
}

void request_timeout_handler() {
	//TODO statemachine reset
	

	printf("Request timed out! Resetting...\n");
	if(UIP_HXB_SEQNUM_MASTER) {
		send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNC);
	} else {
		set_seqnum_invalid();
		send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
	}
}

void send_flag_packet(void* data, int len, uint16_t flags) {
	
	if(flags & UIP_EXT_SEQNUM_FLAG_REQUEST) {
		if(ctimer_expired(&request_timeout_timer)) {
			ctimer_set(&request_timeout_timer,REQUEST_TINMEOUT,&request_timeout_handler,NULL);
		}
	}

#if RAVEN_REVISION == HEXABUS_USB
	uip_len = len + UIP_LLH_LEN + UIP_IPH_LEN + UIP_HBHO_LEN + UIP_EXT_HDR_OPT_EXP_HXB_SEQNUM_LEN;
	uip_udp_packet_send_flags(NULL, data, len, flags);

#else
	if(udpconn == NULL) {
		udpconn = udp_new(NULL, UIP_HTONS(0), NULL);
		udp_bind(udpconn, UIP_HTONS(HXB_PORT));
	}

	uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
	udpconn->rport = UDP_IP_BUF->srcport;
	udpconn->lport = UIP_HTONS(HXB_PORT);

	uip_udp_packet_send_flags(udpconn, data, len, flags);

	memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
	udpconn->rport = 0;
#endif
}

int parse_seqnum_header(int offset) {
	increase_seqnum();

	printf("Got seqnum: %u\n", UIP_SEQNUM_BUF->seqnum);

	if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_SYNC) {
		if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_MASTER) {
			printf("Got sync: ");
			set_seqnum(current_seqnum()-1);
			if(current_seqnum()==UIP_SEQNUM_BUF->flags) {
				set_seqnum_valid();
				printf("Already synced\n");
			} else if(!seqnum_is_valid()) {
				printf("Resyncing to %d\n", UIP_SEQNUM_BUF->seqnum);
				set_seqnum(UIP_SEQNUM_BUF->seqnum);
				set_seqnum_valid();
			} else if(compare_seqnum_less_than(UIP_SEQNUM_BUF->seqnum)) {
				printf("Missing %u packets, requesting %u.\n", abs(UIP_SEQNUM_BUF->seqnum-current_seqnum()), current_seqnum());
				set_seqnum_invalid();
				clear_buffer();

				missing_packets = abs(UIP_SEQNUM_BUF->seqnum-current_seqnum());

				increase_seqnum();
				send_flag_packet(NULL,0,UIP_EXT_SEQNUM_FLAG_REQUEST);
				set_seqnum(current_seqnum()-1);
			} else {
				printf("Resyncing to %d\n", UIP_SEQNUM_BUF->seqnum);
				set_seqnum(UIP_SEQNUM_BUF->seqnum);
				clear_buffer();
				set_seqnum_valid();
			}
		}
		return 1;
	}

	if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_RESYNC) {
		set_seqnum(current_seqnum()-1);

		if(UIP_HXB_SEQNUM_MASTER) {
			printf("Sending sync\n");
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNC);
		}
		return 1;
	}
	
	if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_RESEND) {
		printf("Got resend: ");
		if(missing_packets!=0) {
			if(UIP_SEQNUM_BUF->seqnum==current_seqnum()) {
				printf("Got missing packet %d\n", UIP_SEQNUM_BUF->seqnum);
				missing_packets--;
				ctimer_stop(&request_timeout_timer);

				if(missing_packets!=0) {
					increase_seqnum();
					printf("Still missing %d packets\n", missing_packets);
					send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_REQUEST);
					set_seqnum(current_seqnum()-1);
				}

				return 0;
			} else {
				printf("Not needed, still missing %d packets\n", missing_packets);
				send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_REQUEST);
				set_seqnum(current_seqnum()-1);
				return 1;
			}
		} else {
			printf("Had no missing packets\n");
			set_seqnum(current_seqnum()-1);
			return 1;
		}
	}
	
	if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_REQUEST) {
		uint16_t tmp_seqnum = current_seqnum();
		if(seqnum_is_valid()) {
			void* data=NULL;
			int length;

			printf("Got request: ");
			if(!read_from_buffer(UIP_SEQNUM_BUF->seqnum, data, &length)) {
				printf("Resent %d\n", UIP_SEQNUM_BUF->seqnum);
				set_seqnum(UIP_SEQNUM_BUF->seqnum);
				send_flag_packet(data,length,UIP_EXT_SEQNUM_FLAG_RESEND);
				free(data);
			} else {
				printf("%d not in buffer\n", UIP_SEQNUM_BUF->seqnum);
			}
		}
		set_seqnum(tmp_seqnum-1);
		return 1;
	}

	if(seqnum_is_valid() && (UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_VALID)) {
		if(current_seqnum()==UIP_SEQNUM_BUF->seqnum) {
			//////TEST
			set_seqnum(UIP_SEQNUM_BUF->seqnum-2);
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_REQUEST);
			set_seqnum(current_seqnum()-1);


			return 1;

			//////TEST

			printf("Got correct sequence number.\n");
			return 0;
		} else if(UIP_HXB_SEQNUM_MASTER) {
			printf("Someone is not in sync...sending resync\n");
			set_seqnum(current_seqnum()-1);
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNC);
			return 1;
		} else {
			printf("Got wrong packet, requesting resync\n"); //TODO backoff?
			//set_seqnum_invalid();
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
			return 1;
		}
	} else { //TODO wait for resync
		if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_MASTER && !UIP_HXB_SEQNUM_MASTER) {
			printf("Got packet from master, using sequence number %d\n", UIP_SEQNUM_BUF->seqnum);
			set_seqnum(UIP_SEQNUM_BUF->seqnum); //TODO reset buffer
			set_seqnum_valid();
			return 0;
		} else if(UIP_HXB_SEQNUM_MASTER) {
			printf("Remote seqnum is not valid, syncing...\n");
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNC);
			return 1;		
		} else {
			printf("Own seqnum not valid, resyncing\n");
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);			
			return 1;
		}
	}
}

void resend_init() {
	init_buffer();
	send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
}
