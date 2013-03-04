/*   Copyright (c) 2010 Mathias Dalheimer
 *  All rights reserved.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holders nor the names of
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *Author: 	Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *			Mathias Dalheimer <>
 */

#include "udp_handler.h"
#include <string.h>
#include <stdlib.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "httpd-cgi.h"
#include "packet_builder.h"
#include "lib/crc16.h"
#include <util/delay.h>

#include "state_machine.h"
#include "datetime_service.h"
#include "endpoint_registry.h"
#include "hexabus_config.h"
#include "../../../../../../shared/hexabus_packet.h"

#if UDP_HANDLER_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define UDP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct uip_udp_conn *udpconn;
static struct etimer udp_periodic_timer;

static uip_ipaddr_t hxb_group;

PROCESS(udp_handler_process, "HEXABUS Socket UDP handler Process");
AUTOSTART_PROCESSES(&udp_handler_process);

process_event_t sm_data_received_event;

/*---------------------------------------------------------------------------*/
static void
pollhandler(void) {
  PRINTF("----Socket_UDP_handler: Process polled\r\n");
}

static void
exithandler(void) {
  PRINTF("----Socket_UDP_handler: Process exits.\r\n");
}

void send_packet(char* data, size_t length)
{
  uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr); // reply to the IP from which the request came
  udpconn->rport = UDP_IP_BUF->srcport;
  udpconn->lport = UIP_HTONS(HXB_PORT);
  uip_udp_packet_send(udpconn, data, length);
  PRINTF("%d bytes sent.\r\n", length);

  /* Restore server connection to allow data from any node */
  memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
  udpconn->rport = 0;
}

static void
udphandler(process_event_t ev, process_data_t data)
{
  if (ev == tcpip_event) {
    if(uip_newdata()) {
      PRINTF("udp_handler: received '%d' bytes from ", uip_datalen());
      PRINT6ADDR(&UDP_IP_BUF->srcipaddr);
      PRINTF("\r\n");

      struct hxb_packet_header* header = (struct hxb_packet_header*)uip_appdata;

      // check if it's a Hexabus packet
      if(strncmp(header->header, HXB_HEADER, 4))
      {
        PRINTF("Received something, but it wasn't a Hexabus packet. Ignoring it.");
      }
      else
      {
        if(header->type == HXB_PTYPE_WRITE)
        {
          struct hxb_value value;
          value.datatype = HXB_DTYPE_UNDEFINED;
          uint32_t eid = 0; // FIXME: this is unsafe

          // CRC check and how big the actual value is depend on what type of packet we have.
          switch(header->datatype)
          {
            case HXB_DTYPE_BOOL:
            case HXB_DTYPE_UINT8:
              if(uip_ntohs(((struct hxb_packet_int8*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_int8) - 2, 0))
              {
                PRINTF("CRC check failed.\r\n");
                struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_CRCFAILED);
                send_packet((char*) &error_packet, sizeof(error_packet));
              } else {
                value.datatype = ((struct hxb_packet_int8*)header)->datatype;
                value.v_u8 = ((struct hxb_packet_int8*)header)->value;
                eid = uip_ntohl(((struct hxb_packet_int8*)header)->eid);
              }
              break;
            case HXB_DTYPE_UINT32:
              if(uip_ntohs(((struct hxb_packet_int32*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_int32) - 2, 0))
              {
                PRINTF("CRC check failed.\r\n");
                struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_CRCFAILED);
                send_packet((char*) &error_packet, sizeof(error_packet));
              } else {
                value.datatype = ((struct hxb_packet_int32*)header)->datatype;
                value.v_u32 = uip_ntohl(((struct hxb_packet_int32*)header)->value);
                eid = uip_ntohl(((struct hxb_packet_int32*)header)->eid);
              }
              break;
            case HXB_DTYPE_FLOAT:
              if(uip_ntohs(((struct hxb_packet_float*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_float) - 2, 0))
              {
                PRINTF("CRC check failed.\r\n");
                struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_CRCFAILED);
                send_packet((char*) &error_packet, sizeof(error_packet));
              } else {
                value.datatype = ((struct hxb_packet_float*)header)->datatype;
                uint32_t value_hbo = uip_ntohl(*(uint32_t*)&((struct hxb_packet_float*)header)->value);
                value.v_float = *(float*)&value_hbo;
                eid = uip_ntohl(((struct hxb_packet_float*)header)->eid);
              }
              break;
            case HXB_DTYPE_66BYTES:
              if(uip_ntohs(((struct hxb_packet_66bytes*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_66bytes) - 2, 0))
              {
                PRINTF("CRC check failed.\r\n");
                struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_CRCFAILED);
                send_packet((char*) &error_packet, sizeof(error_packet));
              } else {
                PRINTF("Bytes packet received: ");
                for(int i = 0; i < HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH; i++) {
                  PRINTF("%02x", (((struct hxb_packet_66bytes*)header)->value)[i]);
                }
                eid = uip_ntohl(((struct hxb_packet_66bytes*)header)->eid);
                value.datatype = HXB_DTYPE_66BYTES;
                value.v_binary = (char*) &(((struct hxb_packet_66bytes*)header)->value);
              }
              break;
            case HXB_DTYPE_16BYTES:
              if(uip_ntohs(((struct hxb_packet_16bytes*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_16bytes) - 2, 0))
              {
                PRINTF("CRC check failed.\r\n");
                struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_CRCFAILED);
                send_packet((char*) &error_packet, sizeof(error_packet));
              } else {
                PRINTF("Bytes packet received: ");
                for(int i = 0; i < HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH; i++) {
                  PRINTF("%02x", (((struct hxb_packet_16bytes*)header)->value)[i]);
                }
                eid = uip_ntohl(((struct hxb_packet_16bytes*)header)->eid);
                value.datatype = HXB_DTYPE_16BYTES;
                value.v_binary = (char*) &(((struct hxb_packet_16bytes*)header)->value);
              }
              break;
            default:
              PRINTF("Packet of unknown datatype.\r\n");
              break;
          }

          if(value.datatype != HXB_DTYPE_UNDEFINED) // only continue if actual data was received
          {
            uint8_t retcode = endpoint_write(eid, &value);
            switch(retcode)
            {
              case 0:
                break;    // everything okay. No need to do anything.
              case HXB_ERR_UNKNOWNEID:
              case HXB_ERR_WRITEREADONLY:
              case HXB_ERR_DATATYPE:
              case HXB_ERR_INVALID_VALUE:;
                struct hxb_packet_error error_packet = make_error_packet(retcode);
                send_packet((char*) &error_packet, sizeof(error_packet));
                break;
              default:
                break;
            }
          }
        }
        else if(header->type == HXB_PTYPE_QUERY)
        {
          struct hxb_packet_query* packet = (struct hxb_packet_query*)uip_appdata;
          PRINTF("PACKET EID: %d\n", uip_ntohl(packet->eid));
          // check CRC
          PRINTF("size of packet: %u\n", sizeof(*packet));
          if(uip_ntohs(packet->crc) != crc16_data((unsigned char*)packet, sizeof(*packet)-2, 0))
          {
            PRINTF("CRC check failed.");
            struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_CRCFAILED);
            send_packet((char*) &error_packet, sizeof(error_packet));
          } else
          {
            struct hxb_value value;
            endpoint_read(uip_ntohl(packet->eid), &value);
            if(uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group))
            {
              PRINTF("Group query!\r\n");
              clock_delay_usec(random_rand() >> 1); // wait for max 31ms
            }
            switch(value.datatype)
            {
              case HXB_DTYPE_BOOL:
              case HXB_DTYPE_UINT8:;
                struct hxb_packet_int8 value_packet8 = make_value_packet_int8(uip_ntohl(packet->eid), &value);
                send_packet((char*) &value_packet8, sizeof(value_packet8));
                break;
              case HXB_DTYPE_UINT32:;
                struct hxb_packet_int32 value_packet32 = make_value_packet_int32(uip_ntohl(packet->eid), &value);
                send_packet((char*) &value_packet32, sizeof(value_packet32));
                break;
              case HXB_DTYPE_FLOAT:;
                struct hxb_packet_float value_packetf = make_value_packet_float(uip_ntohl(packet->eid), &value);
                send_packet((char*) &value_packetf, sizeof(value_packetf));
                break;
              case HXB_DTYPE_UNDEFINED:;
                struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_UNKNOWNEID);
                send_packet((char*) &error_packet, sizeof(error_packet));
                break;
              default:
                break;
            }
          }
        }
        else if(header->type == HXB_PTYPE_EPQUERY)
        {
          struct hxb_packet_query* packet = (struct hxb_packet_query*)uip_appdata;
          // check CRC
          PRINTF("size of packet: %u\n", sizeof(*packet));
          if(uip_ntohs(packet->crc) != crc16_data((unsigned char*)packet, sizeof(*packet)-2, 0))
          {
            PRINTF("CRC check failed.");
            struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_CRCFAILED);
            send_packet((char*) &error_packet, sizeof(error_packet));
          } else
          {
            // Check if endpoint exists by reading the datatype and checking the return value
            if(endpoint_get_datatype(uip_ntohl(packet->eid)) == HXB_DTYPE_UNDEFINED) {
              struct hxb_packet_error error_packet = make_error_packet(HXB_ERR_UNKNOWNEID);
              send_packet((char*) &error_packet, sizeof(error_packet));
            } else {
              PRINTF("Sending EndpointInfo packet...\n");
              struct hxb_packet_128string epinfo_packet = make_epinfo_packet(uip_ntohl(packet->eid));
              send_packet((char*) &epinfo_packet, sizeof(epinfo_packet));
            }
          }
        }
        else if(header->type == HXB_PTYPE_INFO)
        {
          struct hxb_envelope* envelope = malloc(sizeof(struct hxb_envelope));
          memcpy(envelope->source, &UDP_IP_BUF->srcipaddr, 16);
          switch(header->datatype)
          {
// Only do this if state_machine is enabled.
#if STATE_MACHINE_ENABLE
              case HXB_DTYPE_BOOL:
              case HXB_DTYPE_UINT8:
                if(uip_ntohs(((struct hxb_packet_int8*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_int8) - 2, 0))
                {
                  PRINTF("CRC Check failed.\r\n");
                  // Broadcast: Don't send an error packet.
                } else {
                  struct hxb_packet_int8* packet = (struct hxb_packet_int8*)header;
                  envelope->eid = uip_ntohl(packet->eid);
                  envelope->value.datatype = packet->datatype;
                  envelope->value.v_u8 = packet->value;
                  process_post(PROCESS_BROADCAST, sm_data_received_event, envelope);
                  PRINTF("Posted event for received broadcast.\r\n");
                }
                break;
              case HXB_DTYPE_UINT32:
                if(uip_ntohs(((struct hxb_packet_int32*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_int32) - 2, 0))
                {
                  PRINTF("CRC Check failed.\r\n");
                  // Broadcast: Don't send an error packet.
                } else {
                  struct hxb_packet_int32* packet = (struct hxb_packet_int32*)header;
                  envelope->eid = uip_ntohl(packet->eid);
                  envelope->value.datatype = packet->datatype;
                  envelope->value.v_u32 = uip_ntohl(packet->value);
                  process_post(PROCESS_BROADCAST, sm_data_received_event, envelope);
                  PRINTF("Posted event for received broadcast.\r\n");
                }
                break;
#endif // STATE_MACHINE_ENABLE
// datetime_service related things should be done when datetime_service is activated
#if DATETIME_SERVICE_ENABLE
              case HXB_DTYPE_DATETIME:
                if(uip_ntohs(((struct hxb_packet_datetime*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_datetime) - 2, 0))
                {
                  PRINTF("CRC Check failed.\r\n");
                  // Broadcast: Don't send an error packet.
                } else {
                  struct hxb_packet_datetime* packet = (struct hxb_packet_datetime*)header;
                  envelope->eid = uip_ntohl(packet->eid);
                  envelope->value.datatype = packet->datatype;
                  packet->value.year = uip_ntohs(packet->value.year);
                  memcpy(&(envelope->value.v_datetime), &(packet->value), sizeof(struct hxb_datetime));
                  // don't post an event here, just call datetime_service. datetime_service also deallocates the memory
                  // process_post(PROCESS_BROADCAST, sm_data_received_event, envelope);
                  // PRINTF("Posted event for received broadcast.\r\n");
                  updateDatetime(envelope);
                }
                break;
#endif // DATETIME_SERVICE_ENABLE
#if STATE_MACHINE_ENABLE
              case HXB_DTYPE_FLOAT:
                if(uip_ntohs(((struct hxb_packet_float*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_float) - 2, 0))
                {
                  PRINTF("CRC Check failed.\r\n");
                  // Broadcast: Don't send an error packet.
                } else {
                  struct hxb_packet_float* packet = (struct hxb_packet_float*)header;
                  envelope->eid = uip_ntohl(packet->eid);
                  envelope->value.datatype = packet->datatype;
                  uint32_t value_hbo = uip_ntohl(*(uint32_t*)&packet->value);
                  envelope->value.v_float = *(float*)&value_hbo;
                  process_post(PROCESS_BROADCAST, sm_data_received_event, envelope);
                  PRINTF("Posted event for received broadcast.\r\n");
                }
                break;
              case HXB_DTYPE_16BYTES:
                if(uip_ntohs(((struct hxb_packet_16bytes*)header)->crc) != crc16_data((unsigned char*)header, sizeof(struct hxb_packet_16bytes) -2, 0))
                {
                  PRINTF("CRC Check failed\r\n");
                } else {
                  struct hxb_packet_16bytes* packet = (struct hxb_packet_16bytes*)header;
                  envelope->eid = uip_ntohl(packet->eid);
                  envelope->value.datatype = packet->datatype;
                  char* b = malloc(HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
                  if(b != NULL) {
                    memcpy(b, &packet->value, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
                    envelope->value.v_binary = b;
                    process_post(PROCESS_BROADCAST, sm_data_received_event, envelope);
                    PRINTF("Posted event for received broadcast.\r\n");
                  } else {
                    PRINTF("UDP Handler: Malloc failed :(\r\n");
                  }
                }
                break;
#endif // STATE_MACHINE_ENABLE
              default:
                PRINTF("Received Broadcast, but no handler for datatype.\r\n");
                break;
          }
        }
        else
        {
          PRINTF("packet of type %d received, but we do not know what to do with that (yet)\r\n", header->type);
        }
        memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
        udpconn->rport = 0;
      }
    }
  }
}

static void print_local_addresses(void) {
  PRINTF("\nAddresses [%u max]\n",UIP_DS6_ADDR_NB);
  int i;
  for (i=0;i<UIP_DS6_ADDR_NB;i++) {
    if (uip_ds6_if.addr_list[i].isused) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
  }
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_handler_process, ev, data) {
//  static uip_ipaddr_t ipaddr;
  PROCESS_POLLHANDLER(pollhandler());
  PROCESS_EXITHANDLER(exithandler());

  // see: http://senstools.gforge.inria.fr/doku.php?id=contiki:examples
  PROCESS_BEGIN();

  sm_data_received_event = process_alloc_event();

  PRINTF("udp_handler: process startup.\r\n");
  // wait 3 second, in order to have the IP addresses well configured
  etimer_set(&udp_periodic_timer, CLOCK_CONF_SECOND*3);
  // wait until the timer has expired
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

  // register the multicast address we want to listen on

  // this wrapper macro is needed to expand HXB_GROUP_RAW before uip_ip6addr, which is a macro
  // it's ugly as day, but it's the least ugly solution i found
  #define PARSER_WRAP(addr, ...) uip_ip6addr(addr, __VA_ARGS__)
  PARSER_WRAP(&hxb_group, HXB_GROUP_RAW);
  #undef PARSER_WRAP
  uip_ds6_maddr_add(&hxb_group);

  udpconn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(udpconn, UIP_HTONS(HXB_PORT));
  // udp_attach(udpconn, NULL);

  PRINTF("udp_handler: Created connection with remote peer ");
  PRINT6ADDR(&udpconn->ripaddr);
  PRINTF("\r\nlocal/remote port %u/%u\r\n", uip_htons(udpconn->lport),uip_htons(udpconn->rport));

  print_local_addresses();
  etimer_set(&udp_periodic_timer, 60*CLOCK_SECOND);

  while(1) {
    //   tcpip_poll_udp(udpconn);
    PROCESS_WAIT_EVENT();
    udphandler(ev, data);
    // Yield to the other processes, so that they have a chance to take broadcast events out of the event queue
    PROCESS_PAUSE();
  }

  PROCESS_END();
}
