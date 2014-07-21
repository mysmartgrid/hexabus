/*
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
 * This file is part of the Contiki operating system.
 *
 */

#include "value_broadcast.h"

#include "contiki.h"
#include "lib/crc16.h"
#include <stdlib.h>
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "sys/ctimer.h"
#include "hexabus_config.h"
#include "state_machine.h"
#include "endpoint_registry.h"
#include "udp_handler.h"
#include "epaper.h"
#include "endpoints.h"

#include "hexabus_packet.h"

#include <stdio.h>
#include <string.h>

#define UDP_EXAMPLE_ID  190

#define LOG_LEVEL VALUE_BROADCAST_DEBUG
#include "syslog.h"

#define SEND_INTERVAL CLOCK_SECOND * VALUE_BROADCAST_AUTO_INTERVAL
#define SEND_TIME (random_rand() % (SEND_INTERVAL))

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

/*---------------------------------------------------------------------------*/
#if VALUE_BROADCAST_AUTO_INTERVAL
PROCESS(value_broadcast_process, "UDP value broadcast sender process");
AUTOSTART_PROCESSES(&value_broadcast_process);
#endif

/*---------------------------------------------------------------------------*/
void broadcast_to_self(struct hxb_value* val, uint32_t eid)
{
#if STATE_MACHINE_ENABLE
  struct hxb_envelope envelope = {
		.src_port = 0,
		.eid = eid,
		.value = *val
 	};
	uip_ip6addr(&envelope.src_ip, 0, 0, 0, 0, 0, 0, 0, 1);
	syslog(LOG_DEBUG, "Sending EID %ld to own state machine.", eid);
	sm_handle_input(&envelope);
#endif
#if EPAPER_ENABLE
	epaper_handle_input(val, eid);
#endif
}

static void broadcast_value_ptr(void* data)
{
	broadcast_value(*(uint32_t*) data);
}

static enum hxb_error_code broadcast_generator(union hxb_packet_any* buffer, void* data)
{
	struct hxb_value val;
	enum hxb_error_code err;

	uint32_t eid = *((uint32_t*) data);

	// link binary blobs and strings
	val.v_string = buffer->p_128string.value;
	if ((err = endpoint_read(eid, &val))) {
		return err;
	}

	buffer->value_header.type = HXB_PTYPE_INFO;
	buffer->value_header.eid = eid;
	buffer->value_header.datatype = val.datatype;

	broadcast_to_self(&val, eid);

	syslog(LOG_DEBUG, "Broadcasting EID %ld, datatype %d", eid, val.datatype);

	switch ((enum hxb_datatype) val.datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			buffer->p_u8.value = val.v_u8;
			break;

		case HXB_DTYPE_UINT32:
		case HXB_DTYPE_TIMESTAMP:
			buffer->p_u32.value = val.v_u32;
			break;

		case HXB_DTYPE_FLOAT:
			buffer->p_float.value = val.v_float;
			break;

		// these just work because value.$blob points to the buffer anyway
		case HXB_DTYPE_16BYTES:
		case HXB_DTYPE_128STRING:
		case HXB_DTYPE_65BYTES:
			break;

		case HXB_DTYPE_DATETIME:
			buffer->p_datetime.value = val.v_datetime;
			break;

		case HXB_DTYPE_UNDEFINED:
		default:
			syslog(LOG_ERR, "Datatype unknown.");
	}

	return HXB_ERR_SUCCESS;
}

void broadcast_value(uint32_t eid)
{
	udp_handler_send_generated(NULL, 0, &broadcast_generator, &eid);
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
	syslog(LOG_DEBUG, "Client IPv6 addresses:");
	for (int i = 0; i < UIP_DS6_ADDR_NB; i++)  {
		uint8_t state = uip_ds6_if.addr_list[i].state;
		if (uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
			syslog(LOG_DEBUG, " " LOG_6ADDR_FMT, LOG_6ADDR_VAL(uip_ds6_if.addr_list[i].ipaddr));
		}
	}
}

void init_value_broadcast(void)
{
  syslog(LOG_INFO, "Value Broadcast init");

  // this wrapper macro is needed to expand HXB_GROUP_RAW before uip_ip6addr, which is a macro
  // it's ugly as day, but it's the least ugly solution i found
  #define PARSER_WRAP(addr, ...) uip_ip6addr(addr, __VA_ARGS__)
  PARSER_WRAP(&server_ipaddr, HXB_GROUP_RAW);
  #undef PARSER_WRAP

  print_local_addresses();

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(HXB_PORT), NULL);
  uip_ipaddr_copy(&client_conn->ripaddr, &server_ipaddr);
  udp_bind(client_conn, UIP_HTONS(HXB_PORT));

	syslog(LOG_DEBUG, "Created a connection " LOG_6ADDR_FMT ", local/remote port %u/%u", LOG_6ADDR_VAL(client_conn->ripaddr), UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));
}

/*---------------------------------------------------------------------------*/
#if VALUE_BROADCAST_AUTO_INTERVAL

PROCESS_THREAD(value_broadcast_process, ev, data)
{
	static struct etimer periodic;
	static uint32_t auto_eids[] = { VALUE_BROADCAST_AUTO_EIDS };
	static struct ctimer backoff_timer[sizeof(auto_eids) / sizeof(auto_eids[0])];

	PROCESS_BEGIN();

	PROCESS_PAUSE();

	init_value_broadcast();

	syslog(LOG_INFO, "UDP sender process started");

	etimer_set(&periodic, SEND_INTERVAL);
	while(1) {
		PROCESS_YIELD();

		if (etimer_expired(&periodic)) {
			etimer_reset(&periodic);

			uint8_t i;
			for (i = 0; i < sizeof(auto_eids) / sizeof(auto_eids[0]); i++) {
				syslog(0, "send %li", auto_eids[i]);
				ctimer_set(&backoff_timer[i], SEND_TIME, broadcast_value_ptr, &auto_eids[i]);
			}
		}
	}

	PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
