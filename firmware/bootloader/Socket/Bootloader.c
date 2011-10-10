/*
 * Bootloader.c
 *
 *  Created on: 08.04.2011
 *      Author: Dennis Wilfert
 */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/fuse.h>
#include <avr/eeprom.h>

#include "spi.h"
#include "eeprom_variables.h"

#include "Bootloader.h"

/* Indicates that the transceiver has received a packet */
extern volatile uint8_t spi_pktReceived;
/* Counter for the timer interrupt */
volatile static uint16_t Counter;
/* Indicates if timer has exceeded the maximum time he should stay in bootloader mode */
volatile static bool timer = false;
/* Indicates if the bootloader has started to write to the flash */
volatile static bool flashing_started = false;

/*
 * Low and high byte of a temporary address
 * This address is generated as a random number when the microcontroler should be flashed
 * and sent with every packet.
 */
uint8_t addr_low;
uint8_t addr_high;

/* Jump address to contiki */
void (*Start_Program)(void)=(void *)0x0000;



/*-------------------------------------------------------------------------*/
/*----------------------Configuration of the .elf file---------------------*/
#if 1
/* The proper way to set the signature is */
#include <avr/signature.h>
#else
/* Older avr-gcc's may not define the needed SIGNATURE bytes. Do it manually if you get an error */
typedef struct {const unsigned char B2;const unsigned char B1;const unsigned char B0;} __signature_t;
#define SIGNATURE __signature_t __signature __attribute__((section (".signature")))
SIGNATURE = {
  .B2 = 0x82,//SIGNATURE_2, //AT90USB1284P
  .B1 = 0x97,//SIGNATURE_1, //128KB flash
  .B0 = 0x1E,//SIGNATURE_0, //Atmel
};
#endif

FUSES ={.low = 0xE2, .high = 0x90, .extended = 0xFF,};


/*----------------------Configuration of EEPROM---------------------------*/
/* Use existing EEPROM if it passes the integrity test, else reinitialize with build values */

volatile uint8_t ee_mem[EESIZE] EEMEM =
{
	0x00, // ee_dummy
	0x02, 0x11, 0x22, 0xff, 0xfe, 0x33, 0x44, 0x12, // ee_mac_addr
	0x00, 0x00, // ee_pan_addr
	0xCD, 0xAB, // ee_pan_id
	0x00, 		//ee_bootloader_flag
	0x00, 0x00,	//ee_bootloader_crc
	0x01,		//ee_first_run
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //ee_encryption_key
	0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ee_domain_name (Socket)
	0xCD, 0x78, //ee_metering_ref (247,4 * 125) = 30925;
	0x64, 0x00, //ee_metering_cal_load (100 W)
	0x00, //ee_metering_cal_flag (0xFF: calibration is unlocked, any other value: locked)
	0x00, //ee_relay_default (0x00: off, 0x01: on)
	0x00, //ee_forwarding (0x00: off, 0x01: forward all incoming traffic);
};

int main(void) {
	ee_mem[0];
    /* Disable watchdog if enabled by bootloader/fuses */
	MCUSR = 0;
	wdt_disable();
	cli();
	// Check if button is pressed, if it is pressed we always go into the bootloader
	if(bit_is_clear(BUTTON_PIN, BUTTON_BIT)) {
		AVR_ENTER_CRITICAL_REGION( );
		eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0x00);
		AVR_LEAVE_CRITICAL_REGION();
	}
	if(eeprom_read_byte((uint8_t *)EE_BOOTLOADER_FLAG)) {
		// Read crc from eeprom
		uint16_t crc;
		AVR_ENTER_CRITICAL_REGION();
		crc = eeprom_read_word ((uint16_t *)EE_BOOTLOADER_CRC);
		AVR_LEAVE_CRITICAL_REGION();
		// Compare crc
		if(crc == calc_crc()) {
			Start_Program();
		}
	}
	hardwareInit();
	transceiver_init();
	timer_init();
	// Enable global interrupts
	sei();

	uint8_t data[30];
	spi_pktReceived = 0;
	// Wait until a timer interrupt occurs or a packet is received
	while(!timer) {
		receivePacket(30, data);	// Wait for a packet that indicates that flashing should be started
		if(data[4] == 'p') {
			addr_low = (uint8_t)rand();		 // Generate a random number
			addr_high = (uint8_t)rand();	 // Generate a random number
			flashing_started = 1;
			ProgramFlash();					 // Start flashing
			/* Enable the watchdog and force a timeout to reset the AVR */
			wdt_enable(WDTO_250MS);

			for (;;);
		}
	}
	/* Enable the watchdog and force a timeout to reset the AVR */
	AVR_ENTER_CRITICAL_REGION( );
	eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0xFF);
	AVR_LEAVE_CRITICAL_REGION();
	wdt_enable(WDTO_250MS);

	for (;;);
}

/*
 * Sets the pan addr of the transceiver
 */
void set_pan_addr(uint16_t pan,uint16_t addr) {
  uint8_t abyte;
  abyte = pan & 0xFF;
  spi_regWrite(0x22,abyte);
  abyte = (pan >> 8*1) & 0xFF;
  spi_regWrite(0x23, abyte);

  abyte = addr & 0xFF;
  spi_regWrite(0x20, abyte);
  abyte = (addr >> 8*1) & 0xFF;
  spi_regWrite(0x21, abyte);
}

/*
 * Hardware initialisation
 */
void hardwareInit(void) {
	/* Relocate the interrupt vector table to the bootloader section */
	MCUCR = (1 << IVCE);
	MCUCR = (1 << IVSEL);
}

/*
 * Initialization of the transceiver
 */
void transceiver_init(void) {
	//DDRC |= 0x0F;
	spi_masterInit();
	irq_init();

	//IO Pins
	DDR_SLPAT |= (1 << SLP_TR); //Enable SLP_TR as output.
	DDR_SLPAT |= (1 << AT_RST); //Enable RST as output.
	DDR_IRQ &= ~(1 << IRQ); //Enable IRQ as output
//	LED_DDR |= _BV(LED);

	//Get the SLP_TR and RST right.to leave P_ON State
	_delay_us(510);
	RST_LOW;
	SLP_TR_LOW;
	_delay_us(1);
	RST_HIGH;
	_delay_us(26);
	//spi_regWrite(0x03, 0x14); // Set clock to 8 MHz
	spi_regWrite(0x02, 0x08); //TRX_OFF
	_delay_us(600);

	// Programming the transceiver
	spi_regWrite(0x08, 0x00); // SAME //Channel 0, 863 Mhz
	//spi_regWrite(0x05, 0xE8);
	spi_regWrite(0x05, 0x62);
	// 100/200/400 kbit/s modes. Spectral side lobes remain < -40 dBm.
	spi_regWrite(0x0C, 0x08); //OQPSK,100kbit/s
	//spi_regWrite(0x0C, 0x24);
	spi_regWrite(0x14, 0x00);

	spi_regWrite(0x09, 0x08),
	spi_regWrite(0x17, 0x40);
	spi_regWrite(0x2C, 0x20);

	spi_regWrite(0x02, 0x09); // State TX_ON
	spi_regWrite( 0x0E, ( ( 0x08 ) ) );
	spi_regWrite(0x2E, 0x00);
	spi_regWrite(0x15, 0x00);

	set_pan_addr(0x0001, 0);
}

/*
 * Initialization of the timer
 */
void timer_init(void) {
	TCCR0B =( 0x00 | ( 1 << CS02 ) | ( 1 << CS00 )); // Teiler: 1024
	TIMSK0 |= ( 1 << TOIE0 );         // Overflow Interrupt einschalten
}
/*
 * Flashing the LED
 */
void flash_led(void)
{
	LED_DDR ^= _BV(LED);

}

/**
 * Programs the flash from an Intel Hex-File. The Hex-File must be transmitted line by line.
 * After each line the microcontroller sends an answer to show if the checksum was
 * correct or not. If the checksum was wrong the line must be resent.
 * If the checksum is correct it returns 0x05 to the host otherwise it returns 0x06 to the host.
 * After the host gets this line he can send the next line or resent the last line.
 * A (german) description of the Intel Hex format can be found here:
 * http://www.schulz-koengen.de/biblio/intelhex.htm
 *
 * @return -1	if a line starts with any other character than ':'
 * 			1	if a line with the "End of File Record" was sent
 */
int ProgramFlash() {

	uint8_t buffer[127];	// Buffer for reading a line
	uint8_t bufferPos;		// Position in buffer

	uint8_t temp;			// Temp variable
	bool nextLine;			// Indicates if there is a next line or not
	uint16_t pagePos;		// Position in the boot-page buffer
	uint8_t dataLength;		// Number of data-bytes of the current line
	uint32_t startAddress;	// Start address for the data of the current data-line
	uint32_t segmentOffset;
	uint16_t currentAddress;// Current address for the data of the current data-line
	uint8_t type;			// Record type of the current line.
	uint8_t checksum;		// In this variable the checksum of one line is calculated
	uint16_t currentpos;
	uint8_t linecount;
	uint8_t wrong_packet;
	uint16_t lastStartAddr;	// Stores the last start addres from a received packet
	bool packetSuccess;		// Indicates if the last packet was successfully written


	packetSuccess = 0;
	lastStartAddr = 0xFFFF;
	startAddress = 0xFFFFFFFFLL;	// Invalid value, because no register is selected
	segmentOffset = 0;
	nextLine = 1;
	pagePos = 0;

	sendAnswer(0x06);

	int l;
	l = 0;
	currentpos = 3;
	while(nextLine) {
		wrong_packet = 0;
		spi_regWrite(0x02, 0x06);	// Go to RX-Mode
		spi_pktReceived = 0;
		uint8_t pktLength;
		// Wait until a packet with the correct address is received
		do {
			pktLength = receivePacket(127, buffer);
			timer = false;
		} while(buffer[1] != addr_low && buffer[2] != addr_high);


		// Read the number of hex-lines that are sent in one packet
		linecount = buffer[3];


		// Checking if the usb-stick sent the same packet again
		// This can happen if he didn't received the answer
		if(( (buffer[6] << 8 ) | buffer[7]) == lastStartAddr && packetSuccess) {
			l = linecount;
		}
		lastStartAddr = temp;
		packetSuccess = 0;
		for(; l < linecount; l++) {

			// Checking if the begin of the line is correct, otherwise we break up.
			if(buffer[currentpos+1] != ':') {
				sendAnswer(0x06);
				bufferPos = 0;
				pagePos -= dataLength;
				wrong_packet = 1;
				break;
			}

			dataLength = buffer[currentpos+2];				// Line length
			checksum = dataLength;

			temp = buffer[currentpos+3];
			checksum += temp;
			currentAddress = ( temp << 8 );					// High byte
			temp = buffer[currentpos+4];
			checksum += temp;
			currentAddress = currentAddress | temp;			// Adding low byte

			type = buffer[currentpos+5];					// Record type
			checksum += type;
			if (currentAddress == 0x0000 && startAddress != 0xFFFFFFFFLL)
				segmentOffset = segmentOffset + 0x10000;
			// Read all bytes of one hex-line for calculating the checksum
			int i;
			for(i = currentpos+6; i < currentpos+6+dataLength; i++){
				checksum += buffer[i];
			}
			// Calculating checksum
			checksum = 0xFF - checksum + 1;
			if(checksum == buffer[currentpos+6+dataLength]) {
				// If the record type indicates that this is the last line.
				if(type == 0x01) {
					nextLine = 0;
					wrong_packet = 0;
					break;
				}

				// Only do something if the line contains data bytes
				if(dataLength) {
					// Setting new startaddress
					if(startAddress == 0xFFFFFFFFLL) {
						// Calculating first position in page
						temp = currentAddress / PAGESIZE;
						startAddress = PAGESIZE * temp;;
						// If we want to write a new page we must first erase the data
						if(pagePos == 0)
							boot_erase_page(startAddress);						// Erase page
					}
					// Startaddress is outside of the current page
					else if(startAddress > currentAddress + segmentOffset || ( startAddress + PAGESIZE ) <= currentAddress + segmentOffset) {
						boot_write_page(startAddress);	// Write the page from the buffer to the flash
						pagePos = 0;
						// Calculating first position in page
						temp = currentAddress / PAGESIZE;
						startAddress = segmentOffset + PAGESIZE * temp;
						// If we want to write a new page we must first erase the data
						if(pagePos == 0)
							boot_erase_page(startAddress);						// Erase page
					}
					pagePos = currentAddress + segmentOffset - startAddress;

					for(bufferPos = currentpos+6; bufferPos < currentpos+dataLength+6; bufferPos+=2, pagePos+=2) {

						// If the current page is full we need to write that page to the flash and start a new one
						if(pagePos >= PAGESIZE) {

							boot_write_page(startAddress);	// Write the page from the buffer to the flash
							startAddress = segmentOffset + currentAddress + bufferPos-6-currentpos;
							pagePos = 0;
							boot_erase_page(startAddress);						// Erase page
						}
						// Setting up two bites to a little-endian word since we have to write words to the buffer
						temp = buffer[bufferPos];
						uint16_t w = temp;

						temp = buffer[bufferPos+1];
						w += (temp) << 8;

						boot_write_buffer(currentAddress, bufferPos-6-currentpos, w);	// Write page buffer
					}
					currentpos = currentpos+dataLength+6;
				}
			}
			// Checksum was not correct
			else {
				sendAnswer(0x06);
				bufferPos = 0;
				pagePos -= dataLength;
				wrong_packet = 1;
				break;
			}
		}
		// Everything went fine so we can say that we need the next packet
		if(!wrong_packet) {
			sendAnswer(0x05); 	// Write back, that everything went fine
			packetSuccess = 1;
			l = 0;
			currentpos = 3;
		}
	}
	boot_write_page(startAddress);	// Write the page from the buffer to the flash

	AVR_ENTER_CRITICAL_REGION();
	eeprom_write_word((uint16_t *)EE_BOOTLOADER_CRC, calc_crc());	// Write crc to eeprom
	eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0xFF); // Write bootloader-flag to eeprom
	AVR_LEAVE_CRITICAL_REGION();
	return 1;
}

/*
 * Sends a packet with a response byte
 * Response-byte is at position 4 the address bytes are on
 * position 5 and 6
 * @param responseByte The response byte that should be sent
 */
void sendAnswer(uint8_t responseByte) {
	uint8_t data[] = { 1, 0, 6, 0, 0, 0, 'X', 'X' };
	data[3] = responseByte;
	data[4] = addr_low;
	data[5] = addr_high;
	spi_regWrite(0x02, 0x09);
	_delay_us(1);
	while(spi_regRead(0x01) != 0x09);
	spi_pktReceived = 0;
	SLP_TR_LOW;

	_delay_us(1);

	SLP_TR_HIGH; //SLPTR_HIGH
	spi_writeFrame(sizeof(data), data);
	while(spi_pktReceived != 1);
}

/*
 * Receiving a packet
 * This method blocks until a packet is received and then reads the packet
 * @param bufferSize	Size of the data buffer
 * @param buffer	 	Buffer in which the packet data is written
 *
 * @return Size of the packet
 */
int receivePacket(uint8_t bufferSize, uint8_t* buffer){
	spi_regWrite(0x02, 0x06);	// Go to RX-Mode
	spi_pktReceived = 0;
	_delay_us(1);
	while(!timer) {
		_delay_us(1);
		if(spi_pktReceived) {
			spi_pktReceived = 0;
			return spi_readFrame(bufferSize, buffer);
		}
	}
	return 0;
}

/**
 * Erasing a page of the flash
 *
 * @param (Byte-) Address of the page
 */
void boot_erase_page(uint32_t address) {
	boot_page_erase(address);
  	boot_spm_busy_wait();
	boot_rww_enable();
	return;
}

/**
 * Writing words to the buffer
 *
 * @param	baseaddr	(Byte-) Address of the page
 * @param	pageaddr	Position (in bytes) in the page
 * @param	word		Word to write
 */
void boot_write_buffer(uint32_t baseaddr, uint16_t pageaddr, uint16_t word) {
	boot_page_fill(baseaddr + pageaddr, word);
	return;
}

/**
 * Writing data from the buffer to the page
 *
 * @param	address		(Byte-) Address of the page
 */
void boot_write_page(uint32_t address) {
	boot_page_write(address);	// Store buffer in flash page.
    boot_spm_busy_wait();		// Wait until the memory is written.
	boot_rww_enable();			// Reenable RWW-section again. We need this if we want to jump back
								// to the application after bootloading.
	return;
}

/*
 * Calculating the CRC of the flash-memory
 */
uint16_t calc_crc(void) {
	uint16_t crc;

	uint16_t c, i;
	prog_uchar *ptr;
	uint8_t byte = 0;
	uint8_t crcbit = 0;
	uint8_t databit = 0;
	ptr = 0;
	// init to all ones
	crc = 0xFFFF;

	for (c = 0; c < BOOTLOADER_START; c++)
	{
		byte = pgm_read_byte(ptr++);

		for (i = 0; i < 8; i++)
		{
		  crcbit = (crc & 0x8000) ? 1 : 0;
		  databit = (byte & 0x80) ? 1 : 0;
		  crc = crc << 1;
		  byte = byte << 1;

	          if (crcbit != databit)
	            crc = crc ^ 0x1021;
		}
	}

	return crc;
}


/*
 * Timer interrupt
 * Sets "timer" to true after Counter == 1000 and stops the
 * timer interrupt.
 */
ISR(TIMER0_OVF_vect) {
	  Counter++;
	  if(Counter%5 == 0)
		  flash_led();
	  if( Counter == 1000 ) { //&& flashing_started == false ) {
	    Counter = 0;
	    timer = true;
	    LED_DDR |= _BV(LED);
	  }
}
