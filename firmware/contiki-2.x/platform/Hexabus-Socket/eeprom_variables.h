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
 * Author: 	Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *
 * @(#)$$
 */

#ifndef EEPROM_VAR_H_
#define EEPROM_VAR_H_

#include <avr/eeprom.h>

enum {
	EEP_SIZE = 4096
};

struct hxb_sm_eeprom_layout {
	/* state machine substructure
	 *
	 * these must always be contiguous in memory, since statemachine upload
	 * writes binary chunks of data without obvious substructure, starting
	 * at statemachine_id[0], and expects that to work. crazy. */
	uint8_t id[16];

	uint8_t code[1024 + 496];
} __attribute__((packed));

struct hxb_eeprom_layout {
	uint8_t dummy;

	/* network configuration */
	uint8_t mac_addr[8];
	uint16_t pan_addr;
	uint16_t pan_id;

	uint8_t bootloader_flag;
	uint16_t bootloader_crc;
	uint8_t first_run;

	uint8_t encryption_key[16];

	char domain_name[30];

	uint16_t metering_ref;
	uint16_t metering_cal_load;
	uint8_t metering_cal_flag;

	uint8_t relay_default;

	uint8_t forwarding;

	struct hxb_sm_eeprom_layout sm;

	/* pad by SIZE - trailer - leader to fill EEP_SIZE exactly */
	uint8_t __padding[EEP_SIZE - 8 - (70 + sizeof(struct hxb_sm_eeprom_layout))];

	uint32_t energy_metering_pulses;
	uint32_t energy_metering_pulses_total;
} __attribute__((packed));

static inline void eeprom_layout_size_static_assert(void)
{
	char bug_on[1 - 2 * !(sizeof(struct hxb_eeprom_layout) == EEP_SIZE)];
}

#define eep_addr(field) ((void*) offsetof(struct hxb_eeprom_layout, field))
#define eep_size(field) (sizeof(((const struct hxb_eeprom_layout*) 0)->field))

#endif /* EEPROM_VAR_H_ */
