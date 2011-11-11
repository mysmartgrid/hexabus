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
 *         Private macro definitions for uIPv6 multicast support
 *
 *         Normally, you won't have to include this directly and you won't have
 *         to invoke any of these macros.
 *
 * \author
 *         George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#ifndef __UIP_MCAST6_PRIVATE_H__
#define __UIP_MCAST6_PRIVATE_H__

#define uip_mcast6_stats         e_stats_prep(UIP_MCAST6_ENGINE_NAME)
#define uip_mcast6_stat          stat_prep(UIP_MCAST6_ENGINE_NAME)

/* Macro Pasting / Prototype Generation */
#define init_prep(e)             init_prep_(e)
#define init_prep_(e)            e##_init()
#define in_prep(e)               in_prep_(e)
#define in_prep_(e)              e##_in()
#define out_prep(e)              out_prep_(e)
#define out_prep_(e)             e##_out()
#define e_stats_prep(e)          e_stats_prep_(e)
#define e_stats_prep_(e)         e##_stats
#define stat_prep(e)             stat_prep_(e)
#define stat_prep_(e)            e##_stat
#define name_str(e)              name_str_(e)
#define name_str_(e)             #e

#endif /* __UIP_MCAST6_PRIVATE_H__ */
