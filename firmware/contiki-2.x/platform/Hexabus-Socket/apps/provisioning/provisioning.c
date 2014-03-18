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
#include "relay.h"


#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


extern uint8_t encryption_enabled;
extern uint32_t mac_framecnt;

#define PROVISIONING_HEADER "!HXB-PAIR"
#define PROVISIONING_REINIT "!HXB-CONN"
#define PROV_TIMEOUT 30

/** \internal The provisioning message structure. */
struct provisioning_m_t {
  char header[sizeof(PROVISIONING_HEADER)-1];
  uint16_t pan_id;
  uint8_t aes_key[16];
};

struct provisioning_r_t {
	char header[sizeof(PROVISIONING_REINIT)-1];
	uint32_t framectr;
};

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

static int query_with_timeout(const void *data, uint16_t len, uint16_t want, uint16_t timeout)
{
	unsigned long start;
	uint16_t rcv;

	start = clock_seconds();
	do {
		provisioning_leds();
		packetbuf_copyfrom(data, len);
		packetbuf_set_datalen(len);
		NETSTACK_RDC.send(NULL, NULL);
		packetbuf_clear();
		packetbuf_set_datalen(0);
		_delay_ms(500);
		rcv = rf212_read(packetbuf_dataptr(), PACKETBUF_SIZE);
		provisioning_leds();
		if (rcv > 0) {
			packetbuf_set_datalen(rcv);
			NETSTACK_FRAMER.parse();
			printf("got %i\n", packetbuf_datalen());
			if (packetbuf_datalen() == want)
				return 0;
		}
	} while (clock_seconds() - start < timeout);

	return -1;
}

int provisioning_slave(void)
{
	leds_off(LEDS_GREEN);
	PRINTF("provisioning: started as slave\n");

	extern uint16_t mac_dst_pan_id;
	extern uint16_t mac_src_pan_id;
	uint16_t old_pan_id = mac_src_pan_id;
	mac_dst_pan_id = 0xffff;
	mac_src_pan_id = 0xffff;
	uint8_t tmp_enc = encryption_enabled;
	encryption_enabled = 0;
	int rc = -1;

	const char *hdr = PROVISIONING_HEADER;
	if (!query_with_timeout(hdr, strlen(hdr), sizeof(struct provisioning_m_t), PROV_TIMEOUT)) {
		struct provisioning_m_t *packet = packetbuf_dataptr();
		PRINTF("provisioning: new pan_id is: %x\n", packet->pan_id);
		PRINTF("provisioning: new pan_id is: %x and new AES_KEY is: ", packet->pan_id);
		int i;
		for (i = 0; i < 16; i++)
			PRINTF("%x ", packet->aes_key[i]);
		PRINTF("\n");
		AVR_ENTER_CRITICAL_REGION();
		eeprom_write_word((uint16_t *)EE_PAN_ID, packet->pan_id);
		eeprom_write_block(packet->aes_key, (void *)EE_ENCRYPTION_KEY, EE_ENCRYPTION_KEY_SIZE);
		AVR_LEAVE_CRITICAL_REGION();
		provisioning_done_leds();
		rf212_key_setup(packet->aes_key);
		old_pan_id = packet->pan_id;
		rf212_set_pan_addr(packet->pan_id, 0, NULL);
		rc = 0;
	}

	mac_dst_pan_id = old_pan_id;
	mac_src_pan_id = old_pan_id;
	encryption_enabled = tmp_enc;
	relay_leds();

	return rc ? rc : provisioning_reconnect();
}

int provisioning_reconnect()
{
	int rc = -1;

	PRINTF("resyncing framectr\n");

	encryption_enabled = 0;

	const char *hdr = PROVISIONING_REINIT;
	if (!query_with_timeout(hdr, strlen(hdr), sizeof(struct provisioning_r_t), 5)) {
		struct provisioning_r_t *rcon = packetbuf_dataptr();

		mac_framecnt = rcon->framectr;
		printf("frame %li\n", mac_framecnt);
		rc = 0;
	}

	relay_leds();
	encryption_enabled = 1;

	PRINTF("sync: %i\n", rc);

	return rc;
}
