/*
 * Copyright (c) 2011, Fraunhofer ESK
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
 *
 * Author: 	G�nter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *
 * @(#)$$
 */
#include "hexabus_config.h"
#include "value_broadcast.h"
#include "endpoints.h"
#include "endpoint_registry.h"

#include "presence_detector.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/ctimer.h"
#include "contiki.h"
#include "dev/leds.h"

#include "nvm.h"

#define LOG_LEVEL PRESENCE_DETECTOR_DEBUG
#include "syslog.h"

#if PRESENCE_DETECTOR_SERVER
static struct ctimer pd_timeout; 
#endif
#if PRESENCE_DETECTOR_CLIENT
static struct ctimer pd_keep_alive;
static uint8_t presence;
#endif

static uint8_t global_presence;


void global_presence_detected(void) {
#if PRESENCE_DETECTOR_SERVER
    if(global_presence != PRESENCE) {
        global_presence = PRESENCE;
        broadcast_value(EP_PRESENCE_DETECTOR);
    }
    ctimer_restart(&pd_timeout);
#endif
}

void no_global_presence(void) {
#if PRESENCE_DETECTOR_SERVER
    global_presence = NO_PRESENCE;
    broadcast_value(EP_PRESENCE_DETECTOR);
#endif
}

void raw_presence_detected(void) {
#if PRESENCE_DETECTOR_CLIENT
    uint8_t tmp = global_presence;
    presence = PRESENCE_DETECTOR_CLIENT_GROUP;
    global_presence = presence;
    broadcast_value(EP_PRESENCE_DETECTOR);
    global_presence = tmp;
    if(PRESENCE_DETECTOR_CLIENT_KEEP_ALIVE != 0) {
        ctimer_restart(&pd_keep_alive);
    }
#endif
}

void no_raw_presence(void) {
#if PRESENCE_DETECTOR_CLIENT
    presence = NO_PRESENCE;
#endif
}

void presence_keep_alive(void* data) {
#if PRESENCE_DETECTOR_CLIENT
    if(presence) {
        raw_presence_detected();
    }
#endif
}

uint8_t is_presence(void) {
    return global_presence;
}

static enum hxb_error_code read(struct hxb_value* value)
{
	value->v_u8 = is_presence();
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write(const struct hxb_envelope* env)
{
	if (env->value.v_u8 == 1) {
		global_presence_detected();
	} else if (env->value.v_u8 == 0) {
		no_raw_presence();
	} else {
		raw_presence_detected();
	}
	return HXB_ERR_SUCCESS;
}

static const char ep_name[] PROGMEM = "Presence Detector";
ENDPOINT_DESCRIPTOR endpoint_presence_detector = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_PRESENCE_DETECTOR,
	.name = ep_name,
	.read = read,
	.write = write
};

void presence_detector_init() {
    global_presence = NO_PRESENCE;
#if PRESENCE_DETECTOR_SERVER
    ctimer_set(&pd_timeout, CLOCK_SECOND*60*PRESENCE_DETECTOR_SERVER_TIMEOUT, no_global_presence, NULL);
    ctimer_stop(&pd_timeout);
#endif
#if PRESENCE_DETECTOR_CLIENT
    presence = NO_PRESENCE;
    if(PRESENCE_DETECTOR_CLIENT_KEEP_ALIVE != 0) {
        ctimer_set(&pd_keep_alive, CLOCK_SECOND*PRESENCE_DETECTOR_CLIENT_KEEP_ALIVE, presence_keep_alive, NULL);
        ctimer_stop(&pd_keep_alive);
    }
#endif
	ENDPOINT_REGISTER(endpoint_presence_detector);
}

