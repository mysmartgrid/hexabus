/*
 * bootloader_send.h
 *
 *  Created on: 20.04.2011
 *      Author: Dennis Wilfert
 */

#ifndef BOOTLOADER_SEND_H_
#define BOOTLOADER_SEND_H_

#include "config.h"

/* Maximum size of an hex-line
 * An array with this size is created.
 * If we get a longer line from the serial
 * port bootloader_send_data() returns with
 * an error. */
#define MAX_HEX_LINE_SIZE 128
/*
 * Numbers of lines to send at once
 */
#define LINE_COUNT 3

int bootloader_send_data(void);


#endif /* BOOTLOADER_SEND_H_ */
