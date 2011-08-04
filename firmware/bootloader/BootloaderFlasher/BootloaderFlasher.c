/*
 * Programming the usb-board with a running bootloader over the serial line
 *
 * BootloaderFlasher.c
 *
 *  Created on: 16.03.2011
 *      Author: Dennis Wilfert
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "librs232.h"
#include "readhex.h"

#include "BootloaderFlasher.h"

int main(int argc, char **argv) {
	char *pvalue = NULL;
	int c;
    opterr = 0;
    bool f = false;
    while ((c = getopt (argc, argv, "hfp:")) != -1)
      switch (c)
        {
      	case 'h':
    	  message();
    	  return 1;
        case 'p':
          pvalue = optarg;
          break;
        case 'f':
        	f = true;
        	break;
        case '?':
          if (optopt == 'c')
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          else {
        	  message();

          }
          return 1;
        default:
          abort ();
        }

    if(optind == 1) {//no option was given
    	message();
    	return -1;
    }

    if(f)
    	return programRS232(argv[optind], pvalue, 'f');

    return programRS232(argv[optind], pvalue, 'p');
}

/*
 * Loads the data from the hex file in the memory,
 * then it initializes the serial-port
 * and then it programs the usb-board over the serial-port.
 * The usb-board must be in Programming mode.
 */
int programRS232(char* path, char* port, unsigned char id) {

	int minaddr, maxaddr, currentaddr;
	unsigned char hash, pre[5], chrRead;

	pre[0] = ':';	// First character of a hex-file line

	// Loading the data from the hex file into the memory in sorted order
	if(load_file(path, &minaddr, &maxaddr) == -1) {
		printf("Loading of the hex-data was unsuccessful\n");
		return -1;
	}

	maxaddr++;	// We need the first byte outside the range

	if(maxaddr <= minaddr) return -1;
	currentaddr = minaddr;

	HRS232 hRS232 = NULL;
	RS232_PARAM param;

	// Opening the serial port
	hRS232 = RS232_Open  ((port == NULL) ? "/dev/ttyACM0" : port);

	if(hRS232==NULL) {
		printf("Can not open rs232-port\n");
		return -1;
	}

	// Setting the parameter (Baudrate can be every value since we use a virtual serial port).
	param.BaudRate = BR_9600;
	param.DataBits = DB_8;
	param.Parity = PARITY_NONE;
	param.StopBits = SB_1;
	param.xonxoff = 0;
	param.rtscts = 0;
	RS232_SetParam(hRS232, &param);

	RS232_Send (hRS232, &id, 1); // Saying the bootloader that we want to start Programming

	chrRead = 0;
	// Bootloader has to answer 0x01, otherwise bootloader is not running or something went wrong
	do {
		readRS232Byte(hRS232, &chrRead,1);
	} while(chrRead != 0x01);
	if(chrRead != 0x01) {
		printf("Bootloader does not answer correctly %d\n", (int)chrRead);
		return -1;
	}

	// Sending the data to the usb-board
	while(currentaddr < maxaddr) {
		pre[1] = (maxaddr-currentaddr < 0x10) ? maxaddr-currentaddr : 0x10;	// We always send 16 bytes (can be changed)
		pre[2] = (char)(currentaddr >> 8);	// High-byte of the address
		pre[3] = (char)(currentaddr % 256);	// Low-byte of the address
		pre[4] = 0x00;	// Record type
		RS232_Send (hRS232, pre, 5);	// Sending the informations to the usb-port
		usleep(10);
		// Calculating the hash
		hash = (pre[1] & 255) + (pre[2] & 255) + (pre[3] & 255) + (pre[4] & 255);
		int i;
		// Sending the 16 data-bytes to the usb-board
		for(i = 0; i < pre[1]; i++) {
			hash += (memory[currentaddr] & 255);
			unsigned char c = memory[currentaddr];
			RS232_Send (hRS232, &c, 1);
			usleep(2);
			currentaddr++;
		}
		hash = 0xFF - hash + 1;
		// Sending the hash to the usb-board
		RS232_Send (hRS232, &hash, 1);
		usleep(10);
		chrRead = 0;
		// Reading the answer from the bootloader
		do {
			readRS232Byte(hRS232, &chrRead,1);
		}while(chrRead != 0x05 && chrRead != 0x06);
		// If everything went fine we getting 0x05 back, otherwise we are sending the line again
		if(chrRead != 0x05) {

			currentaddr -= pre[1];
			printf("Wrong packet %d\n", currentaddr);
		}
		else {
			printf("Writing %02x%02x\n", pre[2], pre[3]);
		}
		usleep(50);

	}
	// Last line of an intel hex file
	unsigned char end[] = {':', 0, 0, 0, 1, 0xFF};
	// Sending the last line to say the bootloader that we have finished sending
	RS232_Send (hRS232, end, 6);
	usleep(50);
	chrRead = 0;
	// The bootloader now must answer 0x05 otherwise something went wrong
	do {
		readRS232Byte(hRS232, &chrRead,1);
	}while(chrRead != 0x05 && chrRead != 0x06);
	if(chrRead != 0x05) {
		printf("Sent everything to the bootloader but he did not exit successfully\n");
		return -1;
	}

	printf("Programming completed successfully\n\n");
	return 1;
}

/*
 * Tries to read a byte from the serial port every 500 μs for max. 500ms.
 * Returns 0 if nothing could be read
 */
RS_UINT32 readRS232Byte (HRS232 hRS232, RS_UINT8 *readbuf, RS_UINT32 readlength) {
	*readbuf = 0;
	int i;
	RS_UINT32 nRead = 0;
	for(i = 0; i < 1000; i++) {
		nRead = RS232_Read(hRS232, readbuf, readlength);
		if(*readbuf != 0)
			return nRead;
		usleep(500); // Wait 500 μs
	}
	return 0;
}

void message() {
	  printf("You need to give the full path to the hex-file that should be programed.\n\n");
	  printf("With the parameter -f you can send the usb-stick with a running contiki into a\n");
	  printf("flash mode, where it flashes a power-supply bootloader.\n");
	  printf("Optional you can give the parameter -p followed by the serial-port (Default is /dev/ttyACM0).\n\n");
	  printf("Example:\n BootloaderFlasher -p /dev/ttyACM0 /home/flash.hex\n\n");
}
