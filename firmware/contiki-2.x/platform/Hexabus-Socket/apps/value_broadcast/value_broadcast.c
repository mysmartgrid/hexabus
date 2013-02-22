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

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "sys/ctimer.h"
#include "hexabus_config.h"
#include "state_machine.h"

#include "../../../../../../shared/hexabus_packet.h"

#include <stdio.h>
#include <string.h>

#define UDP_EXAMPLE_ID  190

#define DEBUG VALUE_BROADCAST_DEBUG
#include "net/uip-debug.h"

#define SEND_INTERVAL CLOCK_SECOND * VALUE_BROADCAST_AUTO_INTERVAL
#define SEND_TIME (random_rand() % (SEND_INTERVAL))

process_event_t immediate_broadcast_event;

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

  struct hxb_envelope* envelope = malloc(sizeof(struct hxb_envelope));
  memset(envelope->source, 0x00,15);
  memset((envelope->source)+15,0x01,1);
  envelope->eid = eid;
  memcpy(&envelope->value, val, sizeof(struct hxb_value));
  process_post(PROCESS_BROADCAST, sm_data_received_event, envelope);
  PRINTF("value_broadcast: Sending EID %ld to own state machine.\n", eid);

#endif
}

void broadcast_value_ptr(void* eidp) { // when called from a callback timer or event, we get a void*
  broadcast_value(*(uint32_t*)eidp);
}

void broadcast_value(uint32_t eid)
{
  struct hxb_value val;
  endpoint_read(eid, &val);

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
      case HXB_DTYPE_UINT8:;
        struct hxb_packet_int8 packet8;
        strncpy(&packet8.header, HXB_HEADER, 4);
        packet8.type = HXB_PTYPE_INFO;
        packet8.flags = 0;
        packet8.eid = uip_htonl(eid);
        packet8.datatype = val.datatype;
        packet8.value = *(uint8_t*)&val.data;
        packet8.crc = uip_htons(crc16_data((char*)&packet8, sizeof(packet8)-2, 0));

        uip_udp_packet_sendto(client_conn, &packet8, sizeof(packet8),
            &server_ipaddr, UIP_HTONS(HXB_PORT));
        break;
      case HXB_DTYPE_UINT32:;
        struct hxb_packet_int32 packet32;
        strncpy(&packet32.header, HXB_HEADER, 4);
        packet32.type = HXB_PTYPE_INFO;
        packet32.flags = 0;
        packet32.eid = uip_htonl(eid);
        packet32.datatype = val.datatype;
        packet32.value = uip_htonl(*(uint32_t*)&val.data);
        packet32.crc = uip_htons(crc16_data((char*)&packet32, sizeof(packet32)-2, 0));

        uip_udp_packet_sendto(client_conn, &packet32, sizeof(packet32),
            &server_ipaddr, UIP_HTONS(HXB_PORT));
        break;
      case HXB_DTYPE_FLOAT:;
        struct hxb_packet_float packetf;
        strncpy(&packetf.header, HXB_HEADER, 4);
        packetf.type = HXB_PTYPE_INFO;
        packetf.flags = 0;
        packetf.eid = uip_htonl(eid);
        packetf.datatype = val.datatype;
        uint32_t value_nbo = uip_htonl(*(uint32_t*)&val.data);
        packetf.value = *(float*)&value_nbo;
        packetf.crc = uip_htons(crc16_data((char*)&packetf, sizeof(packetf)-2, 0));

        uip_udp_packet_sendto(client_conn, &packetf, sizeof(packetf),
            &server_ipaddr, UIP_HTONS(HXB_PORT));
        break;
      case HXB_DTYPE_16BYTES:;
        struct hxb_packet_16bytes packet16;
        strncpy(&packet16.header, HXB_HEADER, 4);
        packet16.type = HXB_PTYPE_INFO;
        packet16.flags = 0;
        packet16.eid = uip_htonl(eid);
        packet16.datatype = val.datatype;
        memcpy(packet16.value, *(void**)&val.data, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
        free(*(void**)&val.data);
        packet16.crc = uip_htons(crc16_data((char*)&packet16, sizeof(packet16)-2, 0));

        uip_udp_packet_sendto(client_conn, &packet16, sizeof(packet16),
          &server_ipaddr, UIP_HTONS(HXB_PORT));
        break;
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
  #define PARSER_WRAP(addr, __VA_ARGS__) uip_ip6addr(addr, __VA_ARGS__)
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

    if(ev == immediate_broadcast_event)
    {
      PRINTF("Value_broadcast: Received immediate_broadcast_event -- EID: %ld\r\n", *(uint32_t*)data);
      broadcast_value_ptr(data);
      free(data);
    }
  }

  PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
