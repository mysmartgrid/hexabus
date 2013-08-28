#include "sequence_number.h"
#include "resend_buffer.h"
#include "net/uip-udp-packet.h"
#include "net/uip.h"
#include "../../../../shared/hexabus_definitions.h"
#include "hexabus_config.h"

#ifndef RESEND_DEBUG
#define RESEND_DEBUG 9
#endif

#define LOG_LEVEL RESEND_DEBUG
#include "syslog.h"

#if RAVEN_REVISION != HEXABUS_USB
	#include "state_machine.h"
	#include "udp_handler.h" 
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern uint8_t uip_ext_len;

static uint16_t sequence_number = 0;
static uint8_t seqnum_valid = 0;
static uint8_t missing_packets = 0;
static struct ctimer request_timeout_timer;

#define REQUEST_TINMEOUT CLOCK_SECOND*2 
#define UDP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_SEQNUM_BUF ((struct uip_ext_hdr_opt_exp_hxb_seqnum *)&uip_buf[uip_l2_l3_hdr_len+offset])

#define STATE_SYNCED 0x01
#define STATE_AWAITING_SYNC 0x02
#define STATE_AWAITING_RESEND 0x03

#if UIP_HXB_SEQNUM_MASTER
	static int resend_state = STATE_SYNCED;
#else
	static int resend_state = STATE_AWAITING_SYNC;
#endif


//TODO Debug/////
static uint32_t dbg_reset_count = 0;
static uint32_t dbg_retransmitted_packets = 0;
static uint32_t dbg_resync_count = 0;
static struct ctimer dbg_timer;
/////////////////


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
	int16_t difference = (int16_t)seqnum - (int16_t)sequence_number;

	if(difference>0)
		return difference;
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

void do_reset() {
	dbg_reset_count++;
#if RAVEN_REVISION != HEXABUS_USB
	syslog(LOG_WARN, "Resetting...");
	process_post(PROCESS_BROADCAST, udp_handler_ready, NULL);
	sm_restart();
#else
	syslog(LOG_WARN, "I'm a USB-Stick, no reset necessary...");
#endif

}

void request_timeout_handler() {
	
	syslog(LOG_DEBUG, "Request timed out!");

	do_reset();
	
	if(UIP_HXB_SEQNUM_MASTER) {
		send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
		resend_state = STATE_SYNCED;
	} else {
		set_seqnum_invalid();
		clear_buffer();
		send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNCREQ);
		resend_state = STATE_AWAITING_SYNC;
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
	uip_udp_packet_sendto_flags(NULL, data, len, NULL, 0, flags);
#else
	udp_handler_send_raw_flags(NULL, 0, flags, len, data);
#endif
}

static int loss = 0;

int parse_seqnum_header(int offset) {

	/*
	//Simulate packet loss
	loss++;
	if (!(loss%20))
	{
		return 1;
	}
	*/

	increase_seqnum();
	syslog(LOG_DEBUG, "Got seqnum: %u", UIP_SEQNUM_BUF->seqnum);
	syslog(LOG_DEBUG, "We have %lu resets, %lu resends and %lu resyncs now.", dbg_reset_count, dbg_retransmitted_packets, dbg_resync_count);

	if(resend_state==STATE_SYNCED) {
		if(!(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_RESEND)) {
			if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_REQUEST) {
				uint16_t tmp_seqnum = current_seqnum();
				if(seqnum_is_valid()) {
					void* data=NULL;
					int length;

					syslog(LOG_DEBUG, "STATE_SYNCED: Got request: ");
					if(!read_from_buffer(UIP_SEQNUM_BUF->seqnum, data, &length)) {
						syslog(LOG_DEBUG, "Resent %d", UIP_SEQNUM_BUF->seqnum);
						set_seqnum(UIP_SEQNUM_BUF->seqnum);
						send_flag_packet(data,length,UIP_EXT_SEQNUM_FLAG_RESEND);
						free(data);
					} else {
						syslog(LOG_DEBUG, "%d not in buffer", UIP_SEQNUM_BUF->seqnum);
					}
				}
				set_seqnum(tmp_seqnum-1);
				return 1;
			} else if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_SYNCREQ) {
				set_seqnum(current_seqnum()-1);
				syslog(LOG_DEBUG, "STATE_SYNCED: Got resync request.");
				if(UIP_HXB_SEQNUM_MASTER) {
					send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
				}
				return 1;

			} else if((UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_RESYNC) && (UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_MASTER)) {
				set_seqnum(current_seqnum()-1);
				if(current_seqnum()==UIP_SEQNUM_BUF->seqnum) {
					syslog(LOG_DEBUG, "STATE_SYNCED: Got correct sync, nothing to do.");
					return 1;
				} else if(compare_seqnum_less_than(UIP_SEQNUM_BUF->seqnum)) {
					missing_packets = compare_seqnum_less_than(UIP_SEQNUM_BUF->seqnum);
					syslog(LOG_DEBUG, "STATE_SYNCED: Missing %d packet(s), requesting...", missing_packets);
					increase_seqnum();
					clear_buffer();
					set_seqnum_invalid();
					resend_state = STATE_AWAITING_RESEND;
					send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_REQUEST);
					set_seqnum(current_seqnum()-1);
					return 1;
				} else {
					syslog(LOG_DEBUG, "STATE_SYNCED: Out of sync, resyncing.");
					dbg_resync_count++;
					clear_buffer();
					set_seqnum(UIP_SEQNUM_BUF->seqnum);
					do_reset();
					return 1;
				}

			} else if((UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_VALID) || (UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_MASTER)) {
				if(current_seqnum()==UIP_SEQNUM_BUF->seqnum) {
					syslog(LOG_DEBUG, "STATE_SYNCED: Got correct packet");
					return 0;
				} else if(compare_seqnum_less_than(UIP_SEQNUM_BUF->seqnum)){
					if(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_MASTER) {
						syslog(LOG_DEBUG, "STATE_SYNCED: Missing packet, requesting...");
						clear_buffer();
						set_seqnum_invalid();
						resend_state = STATE_AWAITING_RESEND;
						send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_REQUEST);
						set_seqnum(current_seqnum()-1);
						return 1;
					} else if(UIP_HXB_SEQNUM_MASTER){
						syslog(LOG_DEBUG, "STATE_SYNCED: Missing packet, sending resync.");
						set_seqnum(current_seqnum()-1);
						send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
						return 1;
					} else {
						syslog(LOG_DEBUG, "STATE_SYNCED: Missing packet, requesting resync.");
						send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNCREQ);
						set_seqnum(current_seqnum()-1);
						return 1;
					}
				} else {
					if(!UIP_HXB_SEQNUM_MASTER) {
						syslog(LOG_DEBUG, "STATE_SYNCED: Out of sync, requesting resync...");
						send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNCREQ);
						set_seqnum(current_seqnum()-1);
						return 1;
					} else {
						syslog(LOG_DEBUG, "STATE_SYNCED: Someone is out of sync, resyncing...");
						set_seqnum(current_seqnum()-1);
						send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
						return 1;
					}
				}
			} else {
				syslog(LOG_DEBUG, "STATE_SYNCED: Got invalid packet, ");
				if(UIP_HXB_SEQNUM_MASTER) {
					syslog(LOG_DEBUG, "sending sync.");
					set_seqnum(current_seqnum()-1);
					send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
					return 1;
				} else {
					syslog(LOG_DEBUG, "ignoring.");
					set_seqnum(current_seqnum()-1);
					return 1;
				}
			}
		}

		set_seqnum(current_seqnum()-1);
		return 1;

	} else if(resend_state==STATE_AWAITING_SYNC) {
		if((UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_RESYNC) && (UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_MASTER)) {
			dbg_resync_count++;
			syslog(LOG_DEBUG, "STATE_AWAITING_SYNC: Got sync");
			set_seqnum(UIP_SEQNUM_BUF->seqnum);
			set_seqnum_valid();
			resend_state = STATE_SYNCED;
			return 1;
		} else if(!(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_SYNCREQ)){
			syslog(LOG_DEBUG, "STATE_AWAITING_SYNC: Reqesting resync...");
			send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_SYNCREQ);
			return 1;
		}
		return 1;

	} else if(resend_state==STATE_AWAITING_RESEND) {
		if((UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_RESEND)&&(UIP_SEQNUM_BUF->flags & UIP_EXT_SEQNUM_FLAG_VALID)) {
				syslog(LOG_DEBUG, "STATE_AWAITING_RESEND: Got resend ");
				if(current_seqnum()==UIP_SEQNUM_BUF->seqnum) {
					missing_packets--;
					ctimer_stop(&request_timeout_timer);
					if(missing_packets>0) {
						increase_seqnum();
						send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_REQUEST);
						set_seqnum(current_seqnum()-1);
					} else {
						resend_state = STATE_SYNCED;
						set_seqnum_valid();
					}

					syslog(LOG_DEBUG, "with correct seqnum, still missing %u packets.", missing_packets);
					dbg_retransmitted_packets++;
					return 0;
				} else {
					syslog(LOG_DEBUG, "with wrong seqnum");
					set_seqnum(current_seqnum()-1);
				}
		}
		set_seqnum(current_seqnum()-1);
		return 1;

	} else {
		syslog(LOG_ERR, "Resend: Wrong state!");
		return 1;
	}
}

#if RAVEN_REVISION!=HEXABUS_USB

PROCESS(resend_init_process, "(Re)initializes the resend protocol.");
AUTOSTART_PROCESSES(&resend_init_process);

PROCESS_THREAD(resend_init_process, ev, data) {

	PROCESS_BEGIN();

	init_buffer();

	bool dbg_data = true;

	while(1) {
		PROCESS_WAIT_EVENT();
		if(ev == udp_handler_ready) {
			syslog(LOG_DEBUG, "Starting initial sync.");
			if(UIP_HXB_SEQNUM_MASTER) {
				resend_state = STATE_SYNCED;
				send_flag_packet(NULL, 0, UIP_EXT_SEQNUM_FLAG_RESYNC);
			} else {
				set_seqnum_invalid();
				clear_buffer();
				resend_state = STATE_AWAITING_SYNC;
				send_flag_packet(&dbg_data, sizeof(bool), UIP_EXT_SEQNUM_FLAG_SYNCREQ);
			}
		}
		PROCESS_PAUSE();
	}
	
	PROCESS_END();
}
#endif