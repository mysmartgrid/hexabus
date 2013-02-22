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

#define EESIZE   4096      // Maximum size of EEPROM
#define EE_DUMMY   		0x000  	                                                	// Dummy element (Address 0 should not be used)
#define EE_MAC_ADDR 		0x001			  										// EUI-64 MAC-Address of the device  (8 Byte)
#define EE_MAC_ADDR_SIZE 		8

#define EE_PAN_ADDR  		( EE_MAC_ADDR + EE_MAC_ADDR_SIZE )	 	        		// 802.15.4 short address (2 Byte)
#define EE_PAN_ADDR_SIZE		2

#define EE_PAN_ID  		( EE_PAN_ADDR + EE_PAN_ADDR_SIZE )	                		// 802.15.4 PAN-ID (2 Byte)
#define EE_PAN_ID_SIZE			2

#define EE_BOOTLOADER_FLAG      ( EE_PAN_ID + EE_PAN_ID_SIZE )                      // Bootloader Flag for reset
#define EE_BOOTLOADER_FLAG_SIZE         1

#define EE_BOOTLOADER_CRC		( EE_BOOTLOADER_FLAG + EE_BOOTLOADER_FLAG_SIZE )	// CRC over image, is checked at startup
#define EE_BOOTLOADER_CRC_SIZE 	2

#define EE_FIRST_RUN_FLAG		( EE_BOOTLOADER_CRC + EE_BOOTLOADER_CRC_SIZE )		//flag which indicates the first startup, it is used for indicate the necessity of provisioning
#define EE_FIRST_RUN_FLAG_SIZE 	1

#define EE_ENCRYPTION_KEY		( EE_FIRST_RUN_FLAG + EE_FIRST_RUN_FLAG_SIZE )		//128bit AES key for encryption
#define EE_ENCRYPTION_KEY_SIZE 16

#define EE_DOMAIN_NAME		( EE_ENCRYPTION_KEY + EE_ENCRYPTION_KEY_SIZE )			// Domain name of Webserver (30 Byte)
#define EE_DOMAIN_NAME_SIZE		30

#define EE_METERING_REF  	( EE_DOMAIN_NAME + EE_DOMAIN_NAME_SIZE )          		// Reference value for power measurement (2 Byte)
#define EE_METERING_REF_SIZE	        2

#define EE_METERING_CAL_LOAD    ( EE_METERING_REF + EE_METERING_REF_SIZE ) 	        // Load power for calibration (2 Byte)
#define EE_METERING_CAL_LOAD_SIZE       2

#define EE_METERING_CAL_FLAG    ( EE_METERING_CAL_LOAD + EE_METERING_CAL_LOAD_SIZE )// Flag which enables/disables calibration via long button click
#define EE_METERING_CAL_FLAG_SIZE       1

#define EE_RELAY_DEFAULT        ( EE_METERING_CAL_FLAG + EE_METERING_CAL_FLAG_SIZE )// Default state of the relay switch
#define EE_RELAY_DEFAULT_SIZE           1

#define EE_FORWARDING        ( EE_RELAY_DEFAULT + EE_RELAY_DEFAULT_SIZE )   		// Flag for enabling forwarding of incoming traffic
#define EE_FORWARDING_SIZE           1

// Data structures for state machine / rule based switching
#define EE_STATEMACHINE_ID                ( EE_FORWARDING + EE_FORWARDING_SIZE )
#define EE_STATEMACHINE_ID_SIZE 16
#define EE_STATEMACHINE_N_CONDITIONS			( EE_STATEMACHINE_ID + EE_STATEMACHINE_ID_SIZE )
#define EE_STATEMACHINE_N_CONDITIONS_SIZE	1
#define EE_STATEMACHINE_CONDITIONS      	( EE_STATEMACHINE_N_CONDITIONS + EE_STATEMACHINE_N_CONDITIONS_SIZE )
#define EE_STATEMACHINE_CONDITIONS_SIZE 	511

#define EE_STATEMACHINE_N_DT_TRANSITIONS					( EE_STATEMACHINE_CONDITIONS + EE_STATEMACHINE_CONDITIONS_SIZE )
#define EE_STATEMACHINE_N_DT_TRANSITIONS_SIZE			1
#define EE_STATEMACHINE_DATETIME_TRANSITIONS 			( EE_STATEMACHINE_N_DT_TRANSITIONS + EE_STATEMACHINE_N_DT_TRANSITIONS_SIZE )
#define EE_STATEMACHINE_DATETIME_TRANSITIONS_SIZE 511

#define EE_STATEMACHINE_N_TRANSITIONS				( EE_STATEMACHINE_DATETIME_TRANSITIONS + EE_STATEMACHINE_DATETIME_TRANSITIONS_SIZE)
#define EE_STATEMACHINE_N_TRANSITIONS_SIZE	1
#define EE_STATEMACHINE_TRANSITIONS     		( EE_STATEMACHINE_N_TRANSITIONS + EE_STATEMACHINE_N_TRANSITIONS_SIZE )
#define EE_STATEMACHINE_TRANSITIONS_SIZE 		495
// =======

#define EE_ENERGY_METERING_PULSES     ( EE_STATEMACHINE_TRANSITIONS + EE_STATEMACHINE_TRANSITIONS_SIZE )
#define EE_ENERGY_METERING_PULSES_SIZE 4

#define EE_ENERGY_METERING_PULSES_TOTAL     ( EE_ENERGY_METERING_PULSES + EE_ENERGY_METERING_PULSES_SIZE )
#define EE_ENERGY_METERING_PULSES_TOTAL_SIZE 4

#endif /* EEPROM_VAR_H_ */

