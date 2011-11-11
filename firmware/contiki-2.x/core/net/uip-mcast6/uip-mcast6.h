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
 *   This header file contains configuration directives for uIPv6
 *   multicast support.
 *
 *   We currently support 2 engines:
 *   - 'Stateless Multicast RPL Forwarding' (SMRF)
 *     RPL does group management as per the RPL docs, SMRF handles datagram
 *     forwarding
 *   - 'Multicast Forwarding with Trickle' according to the algorithm described
 *     in the internet draft:
 *     http://tools.ietf.org/html/draft-ietf-roll-trickle-mcast
 *
 * \author
 *   George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#ifndef __UIP_MCAST6_H__
#define __UIP_MCAST6_H__

#include "contiki-conf.h"
#include "net/uip-mcast6/uip-mcast6-engines.h"
#include "net/uip-mcast6/uip-mcast6-private.h"
#include "net/uip-mcast6/smrf.h"
#include "net/uip-mcast6/roll-trickle.h"
/*---------------------------------------------------------------------------*/
/* Constants */
/*---------------------------------------------------------------------------*/
#define UIP_MCAST6_DROP       0
#define UIP_MCAST6_ACCEPT     1
/*---------------------------------------------------------------------------*/
/* Choose an engine or turn off based on user configuration */
/*---------------------------------------------------------------------------*/
#ifdef UIP_MCAST6_CONF_ENGINE
#define UIP_IPV6_MULTICAST UIP_MCAST6_CONF_ENGINE
#else
#define UIP_IPV6_MULTICAST 0
#endif

#if UIP_IPV6_MULTICAST

/* Configure multicast to play nicely with the engine */
#if UIP_IPV6_MULTICAST == UIP_MCAST6_ENGINE_TRICKLE
#define UIP_IPV6_MULTICAST_RPL 0    /* Not used by trickle */
#define UIP_IPV6_MCAST_TRICKLE 1    /* Turn on mcast HBHO support in uip6.c */
#define UIP_MCAST6_ENGINE_NAME roll_trickle
#elif UIP_IPV6_MULTICAST == UIP_MCAST6_ENGINE_SMRF
#define UIP_IPV6_MULTICAST_RPL 1
#define UIP_MCAST6_ENGINE_NAME smrf
#else
#error "Multicast Enabled with an Unknown Engine."
#error "Check the value of UIP_MCAST6_CONF_ENGINE in conf files."
#endif

/* Stringified engine name for PRINTF happiness */
#define UIP_MCAST6_ENGINE_STR name_str(UIP_MCAST6_ENGINE_NAME)

/* Size of the multicast routing table */
#if UIP_IPV6_MULTICAST_RPL
#ifdef UIP_CONF_DS6_MCAST_ROUTES
#define UIP_DS6_MCAST_ROUTES UIP_CONF_DS6_MCAST_ROUTES
#else
#define UIP_DS6_MCAST_ROUTES 1
#endif /* UIP_CONF_DS6_MCAST_ROUTES */
#endif /* UIP_IPV6_MULTICAST_RPL */
/*---------------------------------------------------------------------------*/
/* Configuration Checks */
/*---------------------------------------------------------------------------*/
#if UIP_IPV6_MULTICAST_RPL && (!UIP_CONF_IPV6_RPL)
#error "The selected Multicast mode requires UIP_CONF_IPV6_RPL != 0"
#error "Check the value of UIP_CONF_IPV6_RPL in conf files."
#endif

#if UIP_IPV6_MULTICAST_RPL && (!UIP_DS6_MCAST_ROUTES)
#error "RPL with Multicast support requires UIP_DS6_MCAST_ROUTES > 0"
#error "Check the value of UIP_CONF_DS6_MCAST_ROUTES in conf files."
#endif
/*---------------------------------------------------------------------------
 * Multicast API. All engines must implement or redefine these functions
 *
 * CPP substitution renames those functions by replacing the 'uip_mcast'
 * prefix with 'engine_name'
 *
 * Thus, to implement an engine called 'foo', one would need to define
 * foo_init() and NOT uip_mcast_init() etc
 *---------------------------------------------------------------------------*/
/**
 * \brief Initialise the 'engine_name' multicast engine
 *
 *  expands to:
 *  void engine_name_init(void);
 */
#define uip_mcast6_init()        init_prep(UIP_MCAST6_ENGINE_NAME)

/**
 * \brief Incoming multicast datagram hook.
 *
 * Process an incoming message and determine whether it should be dropped or
 * accepted. This is where an engine will copy and forward the packet if
 * needed.
 *
 * expands to:
 * uint8_t engine_name_in();
 *
 * \return 0: Drop, 1: Accept
 */
#define uip_mcast6_in()          in_prep(UIP_MCAST6_ENGINE_NAME)

/**
 * \brief Outgoing multicast datagram hook.
 *
 * In this function, the engine gets a chance to process an outgoing packet
 * just before it gets sent. It is where the engine can do things like add an
 * extension header, cache the datagram or even abort transmission by setting
 * uip_len = 0.
 *
 * This hook is only called for UDP datagrams.
 *
 * expands to:
 * void engine_name_out();
 */
#define uip_mcast6_out()         out_prep(UIP_MCAST6_ENGINE_NAME)
/*---------------------------------------------------------------------------
 * Multicast Statistics.
 *
 * For this macro magic to work, an engine should hold its stats in a datatype
 * like this:
 * struct engine_name_stats engine_name_stat;
 *
 *---------------------------------------------------------------------------*/
#ifdef UIP_MCAST6_CONF_STATS
#define UIP_MCAST6_STATS UIP_MCAST6_CONF_STATS
#else
#define UIP_MCAST6_STATS 0
#endif

#if UIP_MCAST6_STATS
extern struct uip_mcast6_stats uip_mcast6_stat;
#define UIP_MCAST6_STAT(s)        uip_mcast6_stat.s
#else
#define UIP_MCAST6_STAT()
#endif

#endif /* UIP_IPV6_MULTICAST */

#endif /* __UIP_MCAST6_H__ */
