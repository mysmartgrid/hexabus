#include "sequence_number.h"
#include "resend_buffer.h"
//#include "uip-udp-packet.h"
#include "contiki-net.h"
#include "../../../../../../shared/hexabus_packet.h"

static uint16_t sequence_number = 0;
static uint8_t seqnum_valid = 0;
static uint8_t missing_packets = 0;
static struct ctimer request_timeout_timer;
static struct uip_udp_conn *udpconn;

#define REQUEST_TINMEOUT CLOCK_SECOND*2 
#define UDP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

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
		send_flag_packet(UIP_EXT_SEQNUM_FLAG_SYNC);
	} else {
		set_seqnum_invalid();
		send_flag_packet(UIP_EXT_SEQNUM_FLAG_RESYNC);
	}
}

void send_flag_packet(void* data, int len, uint16_t flags) {

	uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
	udpconn->rport = UDP_IP_BUF->srcport;
	udpconn->lport = UIP_HTONS(HXB_PORT);

	if(flags & UIP_EXT_SEQNUM_FLAG_RESEND) {
		if(ctimer_expired(&request_timeout_timer)) {
			ctimer_set(&request_timeout_timer,REQUEST_TINMEOUT,&request_timeout_handler,NULL);
		}
	}
	uip_udp_packet_send_flags(udpconn, data, len, flags);

	memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
	udpconn->rport = 0;
}

int parse_seqnum_header(struct uip_ext_hdr_opt_exp_hxb_seqnum* header) {
	increase_seqnum();

	if(header->flags & UIP_EXT_SEQNUM_FLAG_SYNC) {
		if(header->flags & UIP_EXT_SEQNUM_FLAG_MASTER) {
			printf("Got sync: ");
			set_seqnum(current_seqnum()-1);
			if(!seqnum_is_valid() || current_seqnum()!=header->seqnum) {
				printf("Resyncing to %d\n", header->seqnum);
				clear_buffer();
				set_seqnum(header->seqnum);
				set_seqnum_valid();
			} else {
				printf("Already synced\n");
			}
		}
		return 1;
	}

	if(header->flags & UIP_EXT_SEQNUM_FLAG_RESYNC) {
		set_seqnum(current_seqnum()-1);

		if(UIP_HXB_SEQNUM_MASTER) {
			printf("Sending sync\n");
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNC);
		}
		return 1;
	}
	
	if(header->flags & UIP_EXT_SEQNUM_FLAG_RESEND) {
		printf("Got resend: ");
		if(missing_packets!=0) {
			if(header->seqnum==current_seqnum()) {
				printf("Got missing packet %d\n", header->seqnum);
				missing_packets--;
				ctimer_stop(&request_timeout_timer);

				increase_seqnum();
				if(missing_packets!=0) {
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
	
	if(header->flags & UIP_EXT_SEQNUM_FLAG_REQUEST) {
		uint16_t tmp_seqnum = current_seqnum();
		if(seqnum_is_valid()) {
			void* data;
			int length;

			printf("Got request: ");
			if(read_from_buffer(header->seqnum, &data, &length)) {
				printf("Resent %d\n", header->seqnum);
				set_seqnum(header->seqnum);
				send_flag_packet(data,length,UIP_EXT_SEQNUM_FLAG_RESEND);
			} else {
				printf("%d not in buffer\n", header->seqnum);
			}

			free(data);
		}
		set_seqnum(tmp_seqnum-1);
		return 1;
	}

	if(seqnum_is_valid() && (header->flags & UIP_EXT_SEQNUM_FLAG_VALID)) {
		if(current_seqnum()==header->seqnum) {
			printf("Got correct sequence number.\n");
			return 0;
		} else if(compare_seqnum_greater_than(header->seqnum)) {
			printf("Got packet %d, missing %d packets.\n", header->seqnum, header->seqnum-current_seqnum());
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_REQUEST);
			missing_packets = abs(header->seqnum-current_seqnum());
			set_seqnum_invalid();
			clear_buffer();
			set_seqnum(current_seqnum()-1);

			return 1;
		} else {
			printf("Already got packet %d, ignoring.\n", header->seqnum);
			set_seqnum(current_seqnum()-1);
			return 1;
		}
	} else {
		if(header->flags & UIP_EXT_SEQNUM_FLAG_MASTER) {
			printf("Got packet from master, using sequence number %d\n", header->seqnum);
			set_seqnum(header->seqnum);
			set_seqnum_valid();
			return 0;
		} else {
			printf("Got packet, but own or remote sequence number is not valid, ignoring\n");
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
			return 1;		
		}
	}
}
