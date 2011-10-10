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

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/watchdog.h"
#include "usb_drv.h"
#include "usb_descriptors.h"
#include <avr/io.h>
#include <util/delay.h>
#include "sys/clock.h"
#include "serial/uart_usb_lib.h"
#include "sys/etimer.h" //contiki event timer library
#include "dev/leds.h"
#include "radio/rf212bb/rf212bb.h"
#include "radio/rf212bb/hal.h"
#include "frame802154.h"
#include "eeprom_variables.h"
#include "bootloader_send.h"


extern volatile int bootloader_mode;
extern volatile int bootloader_pkt;

/*
 * Getting data from the serial port and sending it to a Socket bootloader
 * to flash it
 */
int bootloader_send_data() {
	watchdog_periodic();
	uint8_t addr_low;
	uint8_t addr_high;
	static uint8_t receive_data[127];
	static uint8_t hex_data[MAX_HEX_LINE_SIZE];
	hex_data[0] = 'p';
	hex_data[1] = 'p';
	hex_data[2] = 'p';
	hex_data[3] = 'p';
	hex_data[4] = 'p';
	hex_data[5] = 'p';
	bootloader_pkt = 0;
	bootloader_mode = 1;
	clock_time_t time;
	extern uint16_t mac_dst_pan_id;
	extern uint16_t mac_src_pan_id;
	uint16_t old_pan_id = mac_src_pan_id;
	//set pan_id for frame creation to 0x0001
	mac_dst_pan_id = 0x0001;
	mac_src_pan_id = 0x0001;

	rf212_set_pan_addr(0x0001, 0, NULL);

	// Saying Socket bootloader that we want to start programming
	rf212_send(hex_data, 5);
	radio_set_trx_state(RX_ON);
	static volatile uint8_t length = 0;
	time = clock_time();
	do {
		while(!bootloader_pkt && (clock_time() - time < CLOCK_SECOND)) {
			watchdog_periodic();
		}
		if (clock_time() - time > CLOCK_SECOND) {
			rf212_send(hex_data, 5);
			radio_set_trx_state(RX_ON); 
			time = clock_time();
		} else {
			length = rf212_read(receive_data, 20);
			bootloader_pkt = 0;
		}			
	} while(length != 6);
	if(receive_data[3] != 0x06) {
		mac_dst_pan_id = old_pan_id;
		mac_src_pan_id = old_pan_id;
		rf212_set_pan_addr(old_pan_id, 0, NULL);
		bootloader_mode = 0;
		while (1);
		return -1;
	}
	addr_low = receive_data[4];
	addr_high = receive_data[5];

	bootloader_pkt = 1;

	// Saying that we are ready to get a line
	usb_stdout_putchar(0x01, 0);

	uart_usb_flush();
	int lastline = 0;
	int l, currentpos;
	uint8_t checksum, checksum_failure;
	while(!lastline) {
		currentpos = 3;
		for(l = 0; l < LINE_COUNT; l++) {
			if(!lastline) {				
				do {
					uint8_t r = uart_usb_getchar();
					if(r == ':') {
						hex_data[currentpos+1] = r;
						r = uart_usb_getchar();
						// Testing if line is to long
						if(r > MAX_HEX_LINE_SIZE - currentpos - 6) {
							mac_dst_pan_id = old_pan_id;
							mac_src_pan_id = old_pan_id;
							rf212_set_pan_addr(old_pan_id, 0, NULL);
							bootloader_mode = 0;
							return -1;
						}
						else if(r == 0){
							lastline = 1;
						}
						checksum = r;
						hex_data[currentpos+2] = r;
						int i;
						// Reading the rest of the line
						for(i = currentpos+3; i < hex_data[currentpos+2]+currentpos+6; i++) {
							hex_data[i] = uart_usb_getchar();
							checksum += hex_data[i];
						}
						hex_data[hex_data[currentpos+2]+currentpos+6] = uart_usb_getchar();
						checksum = 0xFF - checksum + 1;
						if(checksum != hex_data[hex_data[currentpos+2]+currentpos+6]) {
							checksum_failure = 1;
							usb_stdout_putchar(0x06, 0);
							uart_usb_flush();
							lastline = 0;
						}
						else {
							checksum_failure = 0;
							currentpos = hex_data[currentpos+2]+currentpos+6;
							usb_stdout_putchar(0x05, 0);
							uart_usb_flush();
						}

					}
					else {
						usb_stdout_putchar(0x06, 0);
						uart_usb_flush();
						checksum_failure = 1;
					}
					watchdog_periodic();
				}while(checksum_failure);
			}
			else
				break;
		}
		hex_data[1] = addr_low;
		hex_data[2] = addr_high;
		hex_data[3] = l;
		hex_data[currentpos+1] = 'X';
		hex_data[currentpos+2] = 'X';
		hex_data[0] = hex_data[currentpos+2];
		int valid = 0;
		do {
			valid = 0;
			receive_data[3] = 0;
			receive_data[4] = 0;
			receive_data[5] = 0;
			// Sending the full line to the Socket bootloader
			rf212_send(hex_data, currentpos+3);
			radio_set_trx_state(RX_ON);
			time = clock_time();
			bootloader_pkt = 0;			
			while(!bootloader_pkt && (clock_time() - time < CLOCK_SECOND)) {
				watchdog_periodic();
			}
			if(bootloader_pkt){
				bootloader_pkt = 0;
				int length;
				do {
					length = rf212_read(receive_data, 127);
					if(receive_data[4] != addr_low && receive_data[5] != addr_high) {
						while(!bootloader_pkt  && (clock_time() - time < CLOCK_SECOND)){
							watchdog_periodic();
						}
						bootloader_pkt = 0;
					}
					watchdog_periodic();
				} while(receive_data[4] != addr_low && receive_data[5] != addr_high  && (clock_time() - time < CLOCK_SECOND));
				if(receive_data[3] == 0x05)
					valid = 1;
				bootloader_pkt = 1;
			}
		} while(!valid);
	}
	mac_dst_pan_id = old_pan_id;
	mac_src_pan_id = old_pan_id;
	rf212_set_pan_addr(old_pan_id, 0, NULL);
	bootloader_mode = 0;
	watchdog_periodic();
	return 0;
}
