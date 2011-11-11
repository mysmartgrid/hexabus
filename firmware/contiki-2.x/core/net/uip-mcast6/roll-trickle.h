/*
 * Copyright (c) 2011, Loughborough University - Computer Science
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
 *         Header file for IPv6 multicast according to the algorithm in the
 *         "MCAST Forwarding Using Trickle" internet draft.
 *
 *         The current version of the draft can always be found in
 *         http://tools.ietf.org/html/draft-ietf-roll-trickle-mcast
 *
 *         This implementation is based on the draft version stored in
 *         ROLL_TRICKLE_VER
 *
 * \author
 *         George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#ifndef __ROLL_TRICKLE_H__
#define __ROLL_TRICKLE_H__

/*---------------------------------------------------------------------------*/
/* Protocol Constants */
/*---------------------------------------------------------------------------*/
#define ROLL_TRICKLE_VER                    0 /* Supported Draft Version */
#define ROLL_TRICKLE_ICMP_CODE              0 /* ICMPv6 code field */
#define ROLL_TRICKLE_IP_HOP_LIMIT        0xFF /* Hop limit for ICMP messages */
#define ROLL_TRICKLE_INFINITE_REDUNDANCY 0xFF
#define ROLL_TRICKLE_DGRAM_OUT              0
#define ROLL_TRICKLE_DGRAM_IN               1

/*
 * The draft does not currently specify a default number for the trickle
 * interval nor a way to derive it. Examples however hint at 100 msec.
 *
 * In draft terminology, we use an 'aggressive' policy (M=0) and a conservative
 * one (M=1).
 *
 * When experimenting with the two policies on the sky platform,
 * an interval of 125ms proves to be way too low: When we have traffic,
 * doublings happen after the interval end and periodics fire after point T
 * within the interval (and sometimes even after interval end). When traffic
 * settles down, the code compensates the offsets.
 *
 * We consider 125, 250ms etc because they are nice divisors of 1 sec
 * (quotient is power of two). For some machines (e.g sky/msp430,
 * sensinode/cc243x), this is also a nice number of clock ticks
 *
 * After experimentation, the values of Imin leading to best performance are:
 * ContikiMAC: Imin=64 (500ms)
 *   Null RDC: imin=16 (125ms)
 */

/* Configuration for Timer with M=0 (aggressive) */
#ifdef ROLL_TRICKLE_CONF_IMIN_0
#define ROLL_TRICKLE_IMIN_0 ROLL_TRICKLE_CONF_IMIN_0
#else
#define ROLL_TRICKLE_IMIN_0         32 /* 250 msec */
#endif

#ifdef ROLL_TRICKLE_CONF_IMAX_0
#define ROLL_TRICKLE_IMAX_0 ROLL_TRICKLE_CONF_IMAX_0
#else
#define ROLL_TRICKLE_IMAX_0          1 /* Imax = 500ms */
#endif

#ifdef ROLL_TRICKLE_CONF_K_0
#define ROLL_TRICKLE_K_0 ROLL_TRICKLE_CONF_K_0
#else
#define ROLL_TRICKLE_K_0             ROLL_TRICKLE_INFINITE_REDUNDANCY
#endif

#ifdef ROLL_TRICKLE_CONF_T_ACTIVE_0
#define ROLL_TRICKLE_T_ACTIVE_0 ROLL_TRICKLE_CONF_T_ACTIVE_0
#else
#define ROLL_TRICKLE_T_ACTIVE_0      3
#endif

#ifdef ROLL_TRICKLE_CONF_T_DWELL_0
#define ROLL_TRICKLE_T_DWELL_0 ROLL_TRICKLE_CONF_T_DWELL_0
#else
#define ROLL_TRICKLE_T_DWELL_0      11
#endif

/* Configuration for Timer with M=1 (conservative) */
#ifdef ROLL_TRICKLE_CONF_IMIN_1
#define ROLL_TRICKLE_IMIN_1 ROLL_TRICKLE_CONF_IMIN_1
#else
#define ROLL_TRICKLE_IMIN_1         64 /* 500 msec */
#endif

#ifdef ROLL_TRICKLE_CONF_IMAX_1
#define ROLL_TRICKLE_IMAX_1 ROLL_TRICKLE_CONF_IMAX_1
#else
#define ROLL_TRICKLE_IMAX_1          9 /* Imax = 256 secs */
#endif

#ifdef ROLL_TRICKLE_CONF_K_1
#define ROLL_TRICKLE_K_1 ROLL_TRICKLE_CONF_K_1
#else
#define ROLL_TRICKLE_K_1             1
#endif

#ifdef ROLL_TRICKLE_CONF_T_ACTIVE_1
#define ROLL_TRICKLE_T_ACTIVE_1 ROLL_TRICKLE_CONF_T_ACTIVE_1
#else
#define ROLL_TRICKLE_T_ACTIVE_1      3
#endif

#ifdef ROLL_TRICKLE_CONF_T_DWELL_1
#define ROLL_TRICKLE_T_DWELL_1 ROLL_TRICKLE_CONF_T_DWELL_1
#else
#define ROLL_TRICKLE_T_DWELL_1      12
#endif
/*---------------------------------------------------------------------------*/
/* Configuration */
/*---------------------------------------------------------------------------*/
/*
 * Number of Sliding Windows
 * In essence: How many unique sources of simultaneous multicast traffic do we
 * want to support for our lowpan
 * If a node is seeding two multicast streams, parametrized on different M
 * values, then this seed will occupy two different sliding windows
 */
#ifdef ROLL_TRICKLE_CONF_WINS
#define ROLL_TRICKLE_WINS ROLL_TRICKLE_CONF_WINS
#else
#define ROLL_TRICKLE_WINS 2
#endif
/*---------------------------------------------------------------------------*/
/*
 * Maximum Number of Buffered Multicast Messages
 * This buffer is shared across all Seed IDs, therefore a new very active Seed
 * may eventually occupy all slots. It would make little sense (if any) to
 * define support for fewer buffered messages than seeds*2
 */
#ifdef ROLL_TRICKLE_CONF_BUFF_NUM
#define ROLL_TRICKLE_BUFF_NUM ROLL_TRICKLE_CONF_BUFF_NUM
#else
#define ROLL_TRICKLE_BUFF_NUM 6
#endif
/*---------------------------------------------------------------------------*/
/*
 * Use Short Seed IDs [short: 2, long: 16 (default)]
 * It can be argued that we should (and it would be easy to) support both at
 * the same time but the draft doesn't list this as a MUST so we opt for
 * code/ram savings
 */
#ifdef ROLL_TRICKLE_CONF_SHORT_SEEDS
#define ROLL_TRICKLE_SHORT_SEEDS ROLL_TRICKLE_CONF_SHORT_SEEDS
#else
#define ROLL_TRICKLE_SHORT_SEEDS 0
#endif
/*---------------------------------------------------------------------------*/
/*
 * Destination address for our ICMPv6 advertisements. The draft gives us a
 * choice between LL all-nodes or LL all-routers
 *
 * We use allrouters unless a conf directive chooses otherwise
 */
#ifdef ROLL_TRICKLE_CONF_DEST_ALL_NODES
#define ROLL_TRICKLE_DEST_ALL_NODES ROLL_TRICKLE_CONF_DEST_ALL_NODES
#else
#define ROLL_TRICKLE_DEST_ALL_NODES 0
#endif
/*---------------------------------------------------------------------------*/
/*
 * M param for our outgoing messages
 * By default, we set the M bit (conservative). Define this as 0 to clear the
 * M bit in our outgoing messages (aggressive)
 */
#ifdef ROLL_TRICKLE_CONF_SET_M_BIT
#define ROLL_TRICKLE_SET_M_BIT ROLL_TRICKLE_CONF_SET_M_BIT
#else
#define ROLL_TRICKLE_SET_M_BIT 1
#endif
/*---------------------------------------------------------------------------*/
/* Declarations of multicast API functions for the Trickle Multicast engine */
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialise the trickle multicast engine
 */
void roll_trickle_init();

/**
 * \brief Trickle Multicast engine's implementation of a hook for incoming
 * multicast datagrams
 *
 * \return 0: Drop, 1: Accept
 */
#define roll_trickle_in() roll_trickle_accept(ROLL_TRICKLE_DGRAM_IN)

/**
 * \brief Trickle Multicast engine's implementation of a hook for outgoing
 * multicast datagrams
 *
 * \param same as uip_udp_packet_send
 */
void roll_trickle_out();
/*---------------------------------------------------------------------------*/
/* Prototypes of additional Trickle Multicast functions */
/*---------------------------------------------------------------------------*/
/**
 * \brief Called by the uIPv6 engine when it receives a Trickle Multicast
 * ICMPv6 datagram
 */
void roll_trickle_icmp_input();

/**
 * \brief Process an incoming or outgoing multicast message and determine
 * whether it should be dropped or accepted. Cache it if necessary and update
 * trickle timers as required.
 *
 * \param in 1: Incoming packet, 0: Outgoing (we are the seed)
 *
 * \return 0: Drop, 1: Accept
 */
uint8_t roll_trickle_accept(uint8_t in);
/*---------------------------------------------------------------------------*/
/* Stats datatype */
/*---------------------------------------------------------------------------*/
struct roll_trickle_stats {
  uint16_t mcast_in_unique;
  uint16_t mcast_in_all;    /* At layer 3 */
  uint16_t mcast_fwd; /* Forwarded by us but we are not the seed */
  uint16_t mcast_out; /* We are the seed */
  uint16_t mcast_bad;
  uint16_t mcast_dropped;
  uint16_t icmp_in;
  uint16_t icmp_out;
  uint16_t icmp_bad;
};

#endif /* __ROLL_TRICKLE_H__ */
