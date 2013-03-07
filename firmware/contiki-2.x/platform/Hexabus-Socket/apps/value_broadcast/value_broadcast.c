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
#include "packet_builder.h"
#include "endpoint_registry.h"

#include "../../../../../../shared/hexabus_packet.h"

#include <stdio.h>
#include <string.h>

#define UDP_EXAMPLE_ID  190

#define DEBUG VALUE_BROADCAST_DEBUG
#include "net/uip-debug.h"

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
		.source = { 0 },
		.eid = eid,
		.value = *val
 	};
	envelope.source[15] = 1;
  process_post_synch(PROCESS_BROADCAST, sm_data_received_event, &envelope);
  PRINTF("value_broadcast: Sending EID %ld to own state machine.\n", eid);
#endif
}

static void broadcast_value_ptr(void* data)
{
	broadcast_value(*(uint32_t*) data);
}

void broadcast_value(uint32_t eid)
{
	union hxb_packet_any packet;
  struct hxb_value val;

	// link binary blobs and strings
	val.v_string = packet.p_128string.value;
  endpoint_read(eid, &val);

	packet.value_header.type = HXB_PTYPE_INFO;
	packet.value_header.eid = eid;
	packet.value_header.datatype = val.datatype;

  uint32_t localonly[] = { VALUE_BROADCAST_LOCAL_ONLY_EIDS };
  broadcast_to_self(&val, eid);

  int i;
  uint8_t lo = 0;

  for(i=0; i<VALUE_BROADCAST_NUMBER_OF_LOCAL_ONLY_EIDS; i++)
  {
    if(eid == localonly[i])
    {
      lo = 1;
      break;
    }
  }

  if(!lo)
  {
    PRINTF("value_broadcast: Broadcasting EID %ld.\n", eid);
    PRINTF("value_broadcast: Datatype: %d.\n", val.datatype);

    switch(val.datatype)
    {
      case HXB_DTYPE_BOOL:
      case HXB_DTYPE_UINT8:
				packet.p_u8.value = val.v_u8;
				packet_finalize_u8(&packet.p_u8);

				uip_udp_packet_sendto(client_conn, &packet.p_u8, sizeof(packet.p_u8),
						&server_ipaddr, UIP_HTONS(HXB_PORT));
				break;

      case HXB_DTYPE_UINT32:
				packet.p_u32.value = val.v_u32;
				packet_finalize_u32(&packet.p_u32);

				uip_udp_packet_sendto(client_conn, &packet.p_u32, sizeof(packet.p_u32),
						&server_ipaddr, UIP_HTONS(HXB_PORT));
        break;

      case HXB_DTYPE_FLOAT:
				packet.p_float.value = val.v_float;
				packet_finalize_float(&packet.p_float);

				uip_udp_packet_sendto(client_conn, &packet.p_float, sizeof(packet.p_float),
						&server_ipaddr, UIP_HTONS(HXB_PORT));
				break;

      case HXB_DTYPE_16BYTES:
				memcpy(packet.p_16bytes.value, val.v_binary, sizeof(packet.p_16bytes.value));
				packet_finalize_16bytes(&packet.p_16bytes);

				uip_udp_packet_sendto(client_conn, &packet.p_16bytes, sizeof(packet.p_16bytes),
						&server_ipaddr, UIP_HTONS(HXB_PORT));
				break;

			case HXB_DTYPE_DATETIME:
			case HXB_DTYPE_128STRING:
			case HXB_DTYPE_TIMESTAMP:
			case HXB_DTYPE_66BYTES:
			case HXB_DTYPE_UNDEFINED:
      default:
        PRINTF("value_broadcast: Datatype unknown.\r\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++)
  {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
    {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE)
      {
        uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}

void init_value_broadcast(void)
{
  PRINTF("Value Broadcast init\n");

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

  PRINTF("Created a connection");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
  UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));
}

/*---------------------------------------------------------------------------*/
#if VALUE_BROADCAST_AUTO_INTERVAL

PROCESS_THREAD(value_broadcast_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer[VALUE_BROADCAST_NUMBER_OF_AUTO_EIDS];
  static uint32_t auto_eids[] = { VALUE_BROADCAST_AUTO_EIDS };

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  init_value_broadcast();

  PRINTF("UDP sender process started\n");


  etimer_set(&periodic, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&periodic))
    {
      etimer_reset(&periodic);

      uint8_t i;
      for(i = 0 ; i < VALUE_BROADCAST_NUMBER_OF_AUTO_EIDS; i++)
      {
        ctimer_set(&backoff_timer[i], SEND_TIME, broadcast_value_ptr, (void*)&auto_eids[i]);
      }
    }
  }

  PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
