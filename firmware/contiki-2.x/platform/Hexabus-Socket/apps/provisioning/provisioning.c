/*
 * Copyright (c) 2011, Fraunhofer ESK
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Author: 	Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *
 * @(#)$$
 */

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include <avr/io.h>
#include <util/delay.h>
#include "sys/clock.h"
#include "dev/leds.h"
#include "radio/rf212bb/rf212bb.h"
#include "radio/rf212bb/hal.h"
#include "dev/watchdog.h"
#include "net/netstack.h"
#include "packetbuf.h"
#include "eeprom_variables.h"
#include "provisioning.h"
#include <net/mac/sicslowmac.h>
#include "relay.h"
#include "uip.h"


#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


extern uint8_t encryption_enabled;
extern uint16_t mac_dst_pan_id;
extern uint16_t mac_src_pan_id;

enum {
	HXB_B_PAIR_REQUEST = 0,
	HXB_B_PAIR_RESPONSE = 1,

	/* for coexistence with 6lowpan, keep all message tags below 64 */
	HXB_B_TAG = 1,
};

#define PROV_TIMEOUT 30

struct pair_request {
	uint8_t tag;
	uint8_t type;
} __attribute__((packed));

struct pair_response {
	uint8_t tag;
	uint8_t type;
	uint16_t pan_id;
	uint8_t key[16];
	uint64_t key_epoch;
} __attribute__((packed));

//indicates ongoing provisioning
void provisioning_leds(void)
{
	static clock_time_t blink_time;
	static bool red_on = true;
	watchdog_periodic();
	if(clock_time() - blink_time > CLOCK_SECOND / 2) {
		if (red_on) {
			leds_on(LEDS_RED);
		} else {
			leds_off(LEDS_RED);
		}
		red_on = !red_on;
		blink_time = clock_time();
	}
}

//indicates end of provisioning
void provisioning_done_leds(void)
{
	leds_off(LEDS_ALL);
	uint8_t i;
	for(i=0;i<5;i++){
		if (i & 1) {
			leds_on(LEDS_GREEN);
		} else {
			leds_off(LEDS_GREEN);
		}
		watchdog_periodic();
		_delay_ms(500);
	}
}

int provisioning_rx_hook(void* buf, unsigned short len)
{
	if (mac_dst_pan_id == 0xffff) {
		/* drop crc at the end */
		packetbuf_set_datalen(len - 2);
		return 1;
	}

	return 0;
}

static int32_t query_for_params(uint16_t timeout, struct pair_response** resp)
{
	unsigned long start;

	start = clock_seconds();
	do {
		provisioning_leds();

		struct pair_request req = {
			HXB_B_TAG,
			HXB_B_PAIR_REQUEST,
		};

		packetbuf_copyfrom(&req, sizeof(req));
		packetbuf_set_datalen(sizeof(req));
		NETSTACK_RDC.send(NULL, NULL);

		packetbuf_clear();

		for (uint8_t i = 0; i < 100 && !packetbuf_datalen(); i++) {
			_delay_ms(10);
			provisioning_leds();
			while (process_run()) {
			}
		}

		if (packetbuf_datalen() > 0) {
			NETSTACK_RDC.input();
			if (packetbuf_datalen() == sizeof(struct pair_response)) {
				*resp = packetbuf_dataptr();
				return timeout - (clock_seconds() - start);
			}
		}
	} while (clock_seconds() - start < timeout);

	return -1;
}

int provisioning_slave(void)
{
	leds_off(LEDS_GREEN);
	PRINTF("provisioning: started as slave\n");

	uint16_t pan_id = mac_src_pan_id;
	mac_dst_pan_id = 0xffff;
	mac_src_pan_id = 0xffff;
	uint8_t tmp_enc = encryption_enabled;
	encryption_enabled = 0;
	int rc = -1;

	int timeout = PROV_TIMEOUT;
	struct pair_response* resp = 0;

	while (timeout > 0 && rc) {
		timeout = query_for_params(timeout, &resp);

		if (resp->tag != HXB_B_TAG || resp->type != HXB_B_PAIR_RESPONSE)
			continue;

		if (timeout < 0)
			break;

		pan_id = uip_ntohs(resp->pan_id);
		uint64_t key_epoch = resp->key_epoch;

		uint8_t old_key[16];
		uint64_t old_epoch;

		AVR_ENTER_CRITICAL_REGION();
		eeprom_write_word(eep_addr(pan_id), pan_id);

		eeprom_read_block(old_key, eep_addr(encryption_key), 16);
		eeprom_read_block(&old_epoch, eep_addr(llsec_key_epoch), 8);

		eeprom_write_block(resp->key, eep_addr(encryption_key), 16);
		eeprom_write_block(&key_epoch, eep_addr(llsec_key_epoch), 8);

		if (old_epoch != key_epoch || memcmp(resp->key, old_key, 16)) {
			eeprom_write_dword(eep_addr(llsec_frame_counter), 0);
		}
		AVR_LEAVE_CRITICAL_REGION();

		PRINTF("provisioning: joined %04x, key", pan_id);
		for (int i = 0; i < 16; i++)
			PRINTF(" %02x", resp->key[i]);
		PRINTF("\n");

		provisioning_done_leds();
		rc = 0;
	}

	mac_dst_pan_id = pan_id;
	mac_src_pan_id = pan_id;
	rf212_set_pan_addr(pan_id, 0, NULL);
	encryption_enabled = tmp_enc;

	if (!rc)
		watchdog_reboot();

	return rc;
}
