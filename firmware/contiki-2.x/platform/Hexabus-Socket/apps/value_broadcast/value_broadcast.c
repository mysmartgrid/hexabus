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

#include "../../../../../../shared/hexabus_packet.h"

#include <stdio.h>
#include <string.h>

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#define SEND_INTERVAL CLOCK_SECOND * 60
#define SEND_TIME (random_rand() % (SEND_INTERVAL))

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

/*---------------------------------------------------------------------------*/
PROCESS(value_broadcast_process, "UDP value broadcast sender process");
AUTOSTART_PROCESSES(&value_broadcast_process);
/*---------------------------------------------------------------------------*/
// recieving should be handled in a different process
/* static void
tcpip_handler(void)
{
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("DATA recv '%s'\n", str);
  }
} */
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  uint8_t eid = 2;
  struct hxb_value val; 
  endpoint_read(eid, &val);
  PRINTF("value_broadcast: Broadcasting EID %d.\n", eid);
  switch(val.datatype)
  {
    case HXB_DTYPE_BOOL:
    case HXB_DTYPE_UINT8:;
      struct hxb_packet_int8 packet8;
      strncpy(&packet8.header, HXB_HEADER, 4);
      packet8.type = HXB_PTYPE_INFO;
      packet8.flags = 0;
      packet8.eid = eid;
      packet8.datatype = val.datatype;
      packet8.value = val.int8;
      packet8.crc = crc16_data((char*)&packet8, sizeof(packet8)-2, 0);

      uip_udp_packet_sendto(client_conn, &packet8, sizeof(packet8),
                            &server_ipaddr, UIP_HTONS(HXB_PORT));
      break;
    case HXB_DTYPE_UINT32:;
      struct hxb_packet_int32 packet32;
      strncpy(&packet32.header, HXB_HEADER, 4);
      packet32.type = HXB_PTYPE_INFO;
      packet32.flags = 0;
      packet32.eid = eid;
      packet32.datatype = val.datatype;
      packet32.value = val.int32;
      packet32.crc = crc16_data((char*)&packet32, sizeof(packet32)-2, 0);

      uip_udp_packet_sendto(client_conn, &packet32, sizeof(packet32),
                            &server_ipaddr, UIP_HTONS(HXB_PORT));
      break;
    default:
      PRINTF("value_broadcast: Datatype unknown.\r\n");
  }
  

}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
        uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

/* The choice of server address determines its 6LoPAN header compression.
 * (Our address will be compressed Mode 3 since it is derived from our link-local address)
 * Obviously the choice made here must also be selected in udp-server.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the 6LowPAN protocol preferences,
 * e.g. set Context 0 to aaaa::.  At present Wireshark copies Context/128 and then overwrites it.
 * (Setting Context 0 to aaaa::1111:2222:3333:4444 will report a 16 bit compressed address of aaaa::1111:22ff:fe33:xxxx)
 *
 * Note the IPCMV6 checksum verification depends on the correct uncompressed addresses.
 */
 // uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0x0050, 0xc4ff, 0xfe04, 0x000e);
 uip_ip6addr(&server_ipaddr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001); // Link-Local Multicast
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(value_broadcast_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  set_global_address();

  PRINTF("UDP sender process started\n");

  print_local_addresses();

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(HXB_PORT), NULL); 
  uip_ipaddr_copy(&client_conn->ripaddr, &server_ipaddr);
  udp_bind(client_conn, UIP_HTONS(HXB_PORT)); 

  PRINTF("Created a connection");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&periodic, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    /* if(ev == tcpip_event) {
      tcpip_handler();
    } */

    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
