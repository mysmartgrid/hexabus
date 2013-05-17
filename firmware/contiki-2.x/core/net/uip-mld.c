/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *         MLDv1 multicast registration handling (RFC 2710)
 * \author Sean Buckheister	<sean.buckheister@itwm.fhg.de> 
 */

/*
 * (c) Fraunhofer ITWM - Sean Buckheister <sean.buckheister@itwm.fhg.de>, 2012
 *
 * Contiki-MLD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Contiki-MLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Contiki-MLD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "net/uip-ds6.h"
#include "net/uip-icmp6.h"
#include "net/tcpip.h"
#include "lib/random.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#endif

#define UIP_IP_BUF                ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF            ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_ICMP6_ERROR_BUF  ((struct uip_icmp6_error *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define UIP_ICMP6_MLD_BUF  ((struct uip_icmp6_mld1 *)&uip_buf[uip_l2_l3_icmp_hdr_len])

/* Elements 0..multicast_group_count-1 are groups we consider ourselves members of and that we
 * might still need to report */
static uip_ds6_maddr_t *multicast_groups[UIP_DS6_MADDR_NB];
static uint8_t multicast_group_count;

/* Management structures for the MLD responder process */
PROCESS(mld_handler_process, "MLD handler process");
static struct etimer report_timer;

#define MLD_REPORT_ONE_EVENT 0x42
#define MLD_REPORT_ALL_EVENT 0x43


/*---------------------------------------------------------------------------*/
static void
send_mldv1_packet(uip_ip6addr_t *maddr, uint8_t mld_type)
{
  /* IP header */
  /* MLD requires hoplimits to be 1 and source addresses to be link-local.
   * Since routers must send queries from link-local addresses, a link local
   * source be selected.
   * The destination IP must be the multicast group, though, and source address selection
   * will choose a routable address (if available) for multicast groups that are themselves
   * routable. Thus, select the source address before filling the destination.
   **/
  UIP_IP_BUF->ttl = 1;
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
  /* If the selected source is ::, the MLD packet would be invalid. */
  if (uip_is_addr_unspecified(&UIP_IP_BUF->destipaddr)) {
    return;
  }
  
  if (mld_type == ICMP6_ML_REPORT) {
    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, maddr);
  } else {
    uip_create_linklocal_allrouters_mcast(&UIP_IP_BUF->destipaddr);
  }

  UIP_IP_BUF->proto = UIP_PROTO_HBHO;
  uip_len = UIP_LLH_LEN + UIP_IPH_LEN;

  ((uip_hbho_hdr*) &uip_buf[uip_len])->next = UIP_PROTO_ICMP6;
  /* we need only pad with two bytes, so the PadN header is sufficient */
  /* also, len is in units of eight octets, excluding the first. */
  ((uip_hbho_hdr*) &uip_buf[uip_len])->len = (UIP_HBHO_LEN + UIP_RTR_ALERT_LEN + UIP_PADN_LEN) / 8 - 1;
  uip_len += UIP_HBHO_LEN;

  ((uip_ext_hdr_rtr_alert_tlv*) &uip_buf[uip_len])->tag = UIP_EXT_HDR_OPT_RTR_ALERT;
  ((uip_ext_hdr_rtr_alert_tlv*) &uip_buf[uip_len])->len = 2; /* data length of value field */
  ((uip_ext_hdr_rtr_alert_tlv*) &uip_buf[uip_len])->value = 0; /* MLD message */
  uip_len += UIP_RTR_ALERT_LEN;

  ((uip_ext_hdr_padn_tlv*) &uip_buf[uip_len])->tag = UIP_EXT_HDR_OPT_PADN;
  ((uip_ext_hdr_padn_tlv*) &uip_buf[uip_len])->len = 0; /* no data bytes following */
  uip_len += UIP_PADN_LEN;

  uip_len += UIP_ICMPH_LEN;

  uip_ext_len = UIP_HBHO_LEN + UIP_RTR_ALERT_LEN + UIP_PADN_LEN;
  uip_len += UIP_ICMP6_MLD1_LEN;

  UIP_IP_BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);
  UIP_ICMP_BUF->type = mld_type;
  UIP_ICMP_BUF->icode = 0;

  UIP_ICMP6_MLD_BUF->maximum_delay = 0;
  UIP_ICMP6_MLD_BUF->reserved = 0;
  uip_ipaddr_copy(&UIP_ICMP6_MLD_BUF->address, maddr);

  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();
 
  tcpip_ipv6_output();
  UIP_STAT(++uip_stat.icmp.sent);
}

/*---------------------------------------------------------------------------*/
void
uip_icmp6_mldv1_report(uip_ip6addr_t *addr)
{
  if (uip_is_addr_linklocal_allnodes_mcast(addr)) {
    PRINTF("Not sending MLDv1 report for FF02::1\n");
    return;
  }

  PRINTF("Sending MLDv1 report for");
  PRINT6ADDR(addr);
  PRINTF("\n");

  send_mldv1_packet(addr, ICMP6_ML_REPORT);
}

/*---------------------------------------------------------------------------*/
void
uip_icmp6_mldv1_done(uip_ip6addr_t *addr)
{
  if (uip_is_addr_linklocal_allnodes_mcast(addr)) {
    PRINTF("Not sending MLDv1 done for FF02::1\n");
    return;
  }

  PRINTF("Sending MLDv1 done for");
  PRINT6ADDR(addr);
  PRINTF("\n");

  send_mldv1_packet(addr, ICMP6_ML_DONE);
}

/*---------------------------------------------------------------------------*/
void
uip_icmp6_ml_query_input(void)
{
  /*
   * Send an MLDv1 report packet for every multicast address known to be ours.
   */
  PRINTF("Received MLD query from");
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF("for");
  PRINT6ADDR(&UIP_ICMP6_MLD_BUF->address);
  PRINTF("\n");

  if (uip_ext_len == 0) {
    PRINTF("MLD packet without hop-by-hop header received\n");
  } else {
    if (!uip_is_addr_linklocal_allnodes_mcast(&UIP_ICMP6_MLD_BUF->address)
        && uip_ds6_is_my_maddr(&UIP_ICMP6_MLD_BUF->address)) {
      uip_ds6_maddr_lookup(&UIP_ICMP6_MLD_BUF->address)->isreported = 0;
      process_post_synch(&mld_handler_process, MLD_REPORT_ONE_EVENT, NULL);
    } else if (uip_is_addr_unspecified(&UIP_ICMP6_MLD_BUF->address)) {
      process_post_synch(&mld_handler_process, MLD_REPORT_ALL_EVENT, NULL);
    }
    if (etimer_expiration_time(&report_timer) * CLOCK_SECOND > (uint32_t) UIP_ICMP6_MLD_BUF->maximum_delay / 1000) {
      etimer_set(&report_timer, (uint32_t) (random_rand() % UIP_ICMP6_MLD_BUF->maximum_delay) * CLOCK_SECOND / 1000);
    }
  }
}

/*---------------------------------------------------------------------------*/
void
uip_icmp6_ml_report_input(void)
{
  PRINTF("Received MLD report from");
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF("for");
  PRINT6ADDR(&UIP_IP_BUF->destipaddr);
  PRINTF("\n");

  if (uip_ext_len == 0) {
    PRINTF("MLD packet without hop-by-hop header received\n");
  } else if (uip_ds6_is_my_maddr(&UIP_ICMP6_MLD_BUF->address)) {
    uip_ds6_maddr_lookup(&UIP_ICMP6_MLD_BUF->address)->isreported = 1;
  }
}

/*---------------------------------------------------------------------------*/
void
mld_report_now(void)
{
  process_post(&mld_handler_process, MLD_REPORT_ONE_EVENT, NULL);
}

/*---------------------------------------------------------------------------*/
static void
mld_report_init(uint8_t clear_status)
{
  uint8_t m;

  multicast_group_count = 0;
  for (m = 0; m < UIP_DS6_MADDR_NB; m++) {
    if (uip_ds6_if.maddr_list[m].isused) {
      multicast_groups[multicast_group_count] = &uip_ds6_if.maddr_list[m];
      if (clear_status) {
        uip_ds6_if.maddr_list[m].isreported = 0;
      }
      multicast_group_count++;
    }
  }
}

/*---------------------------------------------------------------------------*/
static void
mld_report_one(void)
{
  if (multicast_group_count == 0) {
    etimer_stop(&report_timer);
    return;
  }

  multicast_group_count--;

  if (multicast_groups[multicast_group_count]->isused
      && !multicast_groups[multicast_group_count]->isreported) {
    uip_icmp6_mldv1_report(&multicast_groups[multicast_group_count]->ipaddr);
  }

  etimer_reset(&report_timer);
}

/*---------------------------------------------------------------------------*/
void
uip_mld_init(void)
{
  process_start(&mld_handler_process, NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mld_handler_process, ev, data)
{
  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER) {
      mld_report_one();
    } else if (ev == MLD_REPORT_ONE_EVENT || ev == MLD_REPORT_ALL_EVENT) {
      mld_report_init(ev == MLD_REPORT_ALL_EVENT);
      etimer_set(&report_timer, 0);
    }
  }

  PROCESS_END();
}

/** @} */
