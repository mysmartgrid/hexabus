/*
 * BootloaderFlasher.h
 *
 *  Created on: 23.03.2011
 *      Author: Dennis Wilfert
 */

#ifndef BOOTLOADERFLASHER_H_
#define BOOTLOADERFLASHER_H_

RS_UINT32 readRS232Byte (HRS232 hRS232, RS_UINT8 *readbuf, RS_UINT32 readlength);
int programRS232(char* path, char* port, unsigned char id);
void message();

#endif /* BOOTLOADERFLASHER_H_ */
