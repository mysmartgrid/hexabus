/*
 * Copyright (c) 2010, Loughborough University - Computer Science
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
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         This file implements 'Stateless Multicast RPL Forwarding' (SMRF)
 *
 *         It will only work in RPL networks in MOP 3 "Storing with Multicast"
 *
 * \author
 *         George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "net/uip-mcast6/uip-mcast6.h"
#include "net/uip-mcast6/smrf.h"
#include "net/rpl/rpl.h"
#include "net/netstack.h"
#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/uip-debug.h"

/*---------------------------------------------------------------------------*/
/* Macros */
/*---------------------------------------------------------------------------*/
/* CCI */
#define SMRF_FWD_DELAY()  NETSTACK_RDC.channel_check_interval()
/* Number of slots in the next 500ms */
#define SMRF_INTERVAL_COUNT  ((CLOCK_SECOND >> 2) / fwd_delay)
/*---------------------------------------------------------------------------*/
/* Internal Data */
/*---------------------------------------------------------------------------*/
static struct ctimer mcast_periodic;
static uint8_t mcast_len;
static uip_buf_t mcast_buf;
static uint8_t fwd_delay;
static uint8_t fwd_spread;

/* Maintain Stats */
#if UIP_MCAST6_STATS
struct smrf_stats smrf_stat;
#define STATS_ADD(x) smrf_stat.x++
#define STATS_RESET() do { \
  memset(&smrf_stat, 0, sizeof(smrf_stat)); } while(0)
#else
#define STATS_ADD(x)
#define STATS_RESET()
#endif
/*---------------------------------------------------------------------------*/
/* uIPv6 Pointers */
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
/*---------------------------------------------------------------------------*/
static void
mcast_fwd(void * p)
{
  memcpy(uip_buf, &mcast_buf, mcast_len);
  uip_len = mcast_len;
  UIP_IP_BUF->ttl--;
  tcpip_output(NULL);
  uip_len = 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
smrf_in()
{
  rpl_dag_t * d; /* Our DODAG */
  uip_ds6_nbr_t * p; /* Our pref. parent's LL address */

  /*
   * Fetch a pointer to the LL address of our preferred parent
   *
   * ToDo: This rpl_get_any_dag() call is a dirty replacement of the previous
   *   rpl_get_dag(RPL_DEFAULT_INSTANCE);
   * so that things can compile with the new RPL code. This needs updated to
   * read instance ID from the RPL HBHO and use the correct parent accordingly
   *
   * ToDO: This will fail if our preferred parent's entry falls off the
   * ND cache. This may happen if our neighbour count is greater
   * than UIP_CONF_DS6_NBR_NB. Result: risk discarding packets that we should
   * have accepted because we didn't realise they came from our Pref. Par.
   */
  d = rpl_get_any_dag();
  if(!d) {
    STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  p = uip_ds6_nbr_lookup(&d->preferred_parent->addr);
  if(!p) {
    STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  /*
   * We accept a datagram if it arrived from our preferred parent, discard
   * otherwise.
   */
  if(memcmp(p->lladdr.addr, packetbuf_addr(PACKETBUF_ADDR_SENDER),
      UIP_LLADDR_LEN)){
    PRINTF("SMRF: Routable in but RPL ignored it\n");
    STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  if(UIP_IP_BUF->ttl <= 1) {
    STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  STATS_ADD(mcast_in_all);
  STATS_ADD(mcast_in_unique);

  /* If we have an entry in the mcast routing table, something with
   * a higher RPL rank (somewhere down the tree) is a group member */
  if(uip_ds6_mcast_route_lookup(&UIP_IP_BUF->destipaddr)) {
    /* If we enter here, we will definitely forward */
    STATS_ADD(mcast_fwd);

    /*
     * Add a delay (D) of at least SMRF_FWD_DELAY() to compensate for how
     * contikimac handles broadcasts. We can't start our TX before the sender
     * has finished its own.
     */
    fwd_delay = SMRF_FWD_DELAY();

    /* Finalise D: D = min(SMRF_FWD_DELAY(), SMRF_MIN_FWD_DELAY) */
#if SMRF_MIN_FWD_DELAY
    if(fwd_delay < SMRF_MIN_FWD_DELAY) { fwd_delay = SMRF_MIN_FWD_DELAY; }
#endif

    if(fwd_delay == 0) {
      /* No delay required, send it, do it now, why wait? */
      UIP_IP_BUF->ttl--;
      tcpip_output(NULL);
      UIP_IP_BUF->ttl++; /* Restore before potential upstack delivery */
    } else {
      /* Randomise final delay in [D , D*Spread], step D */
      fwd_spread = SMRF_INTERVAL_COUNT;
      if(fwd_spread > SMRF_MAX_SPREAD) {
        fwd_spread = SMRF_MAX_SPREAD;
      }
      if(fwd_spread) {
        fwd_delay = fwd_delay
            * (1 + ((random_rand() >> 11) % fwd_spread));
      }

      memcpy(&mcast_buf, uip_buf, uip_len);
      mcast_len = uip_len;
      ctimer_set(&mcast_periodic, fwd_delay, mcast_fwd, NULL);
    }
    PRINTF("SMRF: %u bytes: fwd in %u [%u]\n",
        uip_len, fwd_delay, fwd_spread);
  }

  /* Done with this packet unless we are a member of the mcast group */
  if(!uip_ds6_is_my_maddr(&UIP_IP_BUF->destipaddr)) {
    PRINTF("SMRF: Not a group member. No further processing\n");
    return UIP_MCAST6_DROP;
  } else {
    STATS_ADD(mcast_in_ours);
    return UIP_MCAST6_ACCEPT;
  }
}
