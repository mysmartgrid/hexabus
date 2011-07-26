/*
 * Bootloader.h
 *
 *  Created on: 08.04.2011
 *      Author: Dennis Wilfert
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

//Status LED
 #define LED_DDR  DDRC
 #define LED_PORT PORTC
 #define LED_PIN  PINC
 #define LED      PC0

#define PAGESIZE	256U	/**< Size (in Bytes) of a flash page */
#define BOOTLOADER_START	0xF000	/**< Start of the bootloader section */

// Button definitions
#define BUTTON_PORT		PORTD
#define BUTTON_PIN		PIND
#define BUTTON_BIT		PD5
#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))

void transceiver_init(void);
void timer_init(void);
void sendAnswer(uint8_t responseByte);
int ProgramFlash(void);
int receivePacket(uint8_t bufferSize, uint8_t* buffer);
void hardwareInit(void);

void boot_erase_page(uint32_t address);
void boot_write_buffer(uint32_t baseaddr, uint16_t pageaddr, uint16_t word);
void boot_write_page(uint32_t address);
void set_pan_addr(uint16_t pan,uint16_t addr);

void flash_led(void);
uint16_t calc_crc(void);
#endif /* BOOTLOADER_H_ */
