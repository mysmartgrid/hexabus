#include "at45.h"
#include "_spi.h"
#include <avr/io.h>

// Command words from datasheet, p. 28ff.
#define B1_TO_MM_PAGE_PROG_W_ERASE 0x83
#define MM_PAGE_TO_B1_XFER 0x53
#define BUFFER_1_READ 0xd4
#define BUFFER_1_WRITE 0x84
#define STATUS_REG_READ 0xd7

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdlib.h>

void at45_select(void) { 
  AT45_CS_PORT &= ~(1 << AT45_CS); 
} 

void at45_deselect(void) { 
  AT45_CS_PORT |= (1 << AT45_CS); 
} 

// at45db161d read/write 
uint8_t at45_rw(uint8_t data) { 
  return spi_rw(data);
}

void at45_init()
{
	AT45_CS_PORT  |= (1 << AT45_CS);
	AT45_CS_DDR   |= (1 << AT45_CS);
}

static uint8_t sck_port, mosi_ddr, sck_ddr, miso_ddr, spcr, spsr;

// initialisation of SPI towards the AT45 chip 
void at45_acquire(void) { 
	sck_port = AT45_SCK_PORT;
	mosi_ddr = AT45_MOSI_DDR;
	sck_ddr = AT45_SCK_DDR;
	miso_ddr = AT45_MISO_DDR;
	spcr = SPCR;
	spsr = SPSR;
  // Configure output pins.
  AT45_SCK_PORT |= (1 << AT45_SCK); 
  AT45_MOSI_DDR |= (1 << AT45_MOSI);
  AT45_SCK_DDR  |= (1 << AT45_SCK); 
  // Configure input pins.
  AT45_MISO_DDR &= ~(1 << AT45_MISO); 
  // freq SPI CLK/2 

 // SPCR = (1<<SPE)|(1<<MSTR); 
 // SPSR |= (1<<SPI2X); 

 // set SPI rate = CLK/64 
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1); 
  SPSR &= ~(1<<SPI2X); 
}

void at45_release()
{
	AT45_SCK_PORT = sck_port;
	AT45_MOSI_DDR = mosi_ddr;
	AT45_SCK_DDR = sck_ddr;
	AT45_MISO_DDR = miso_ddr;
	SPCR = spcr;
	SPSR = spsr;
}

void at45_erase_all_pages(void) {
  at45_select(); 
  at45_rw(0xc7); 
  at45_rw(0x94); 
  at45_rw(0x80); 
  at45_rw(0x9a); 
  at45_deselect(); 
}

void at45_get_version(struct at45_version_t* version) {
  at45_select(); 
  at45_rw(0x9f); 
  version->data[0] = at45_rw(0x00); 
  version->data[1] = at45_rw(0x00); 
  version->data[2] = at45_rw(0x00); 
  version->data[3] = at45_rw(0x00); 
  at45_deselect(); 
}


uint8_t at45_read_page(struct at45_page_t* page, uint16_t addr_page) {
  //uart_puts_P("Step 1: Reading page from buffer1.\r\n");
  at45_read_from_buf_1(page->data, AT45_PAGE_SIZE, 0);
  at45_wait_ready();
  if (! at45_read_page_to_buf_1(addr_page)) {
	return 0;
  } else {
	at45_wait_ready();
	//uart_puts_P("Step 2: Reading page from buffer1.\r\n");
	at45_read_from_buf_1(page->data, AT45_PAGE_SIZE, 0);
	return 1;
  }
}

// what's our status? 
uint8_t at45_status(void) { 
  uint8_t result=0x00; 
  at45_select(); 
  at45_rw(STATUS_REG_READ); 
  // the status is clocked out continuously as data written to SO. So:
  // do dummy writes.
  at45_rw(0xFF); 
  result = at45_rw(0xFF); 
  at45_deselect(); 
  return result; 
} 

uint8_t at45_is_ready(void) {
  uint8_t result = at45_status();
  return (result & (1 << 7));
}

uint8_t at45_wait_ready(void) {
  uint8_t result = 0;
  while (! (result = at45_is_ready())) ;;
  return result;
}

// page to buffer 
uint8_t at45_read_page_to_buf_1(uint16_t addr_page) { 
  // 4096 - number of pages per AT45DB161D 
  if(addr_page < 4096) 
  { 
	at45_select(); 
	at45_rw(MM_PAGE_TO_B1_XFER); 
	at45_rw((uint8_t) (((addr_page << 2) & 0x3f00) >> 8) );
	at45_rw((uint8_t) ((addr_page << 2) & 0x00fc) );
	at45_rw(0x00); 
	at45_deselect(); 
	return 1; 
  } 
  return 0; 
} 

// copy from the buffer over to the chip 
uint8_t at45_write_from_buf_1(uint16_t addr_page) __attribute__ ((optimize(1))); 

uint8_t at45_write_from_buf_1(uint16_t addr_page) { 
  // 4096 - number of pages AT45DB161D 
  if(addr_page < 4096) 
  { 
	at45_select(); 
	at45_rw(B1_TO_MM_PAGE_PROG_W_ERASE); // write data from buffer1 to page 
	at45_rw((uint8_t) (((addr_page << 2) & 0x3f00) >> 8));
	at45_rw((uint8_t) ((addr_page << 2) & 0x00fc));
	at45_rw(0x00); 
	at45_deselect(); 
	return 0; 
  } 
  return 1; 
} 

// copying from BUFFER1 to RAM 
void at45_read_from_buf_1(uint8_t * dst, uint16_t count, uint16_t addr) { 
  uint16_t i = 0; 
//  char conversion_buffer[50];
  at45_select(); 
  at45_rw(BUFFER_1_READ);                  
  at45_rw(0x00); 
  at45_rw(0x00); 
  at45_rw(0x00); 
  at45_rw(0x00); 
  while(count--) {
	dst[i++] = at45_rw(0xFF); 
	//itoa(dst[i-1], conversion_buffer, 16);
	//uart_puts(conversion_buffer);
	//uart_puts_P(":");
  }
  at45_deselect(); 
} 

// transferring the data over to BUFFER 1 
void at45_write_to_buf_1(const uint8_t *src, uint16_t count, uint16_t addr) __attribute__ ((optimize(1))); 

void at45_write_to_buf_1(const uint8_t *src, uint16_t count, uint16_t addr) { 
  uint16_t i = 0; 
  at45_select(); 
  at45_rw(BUFFER_1_WRITE);                  
  at45_rw(0x00); 
  at45_rw((uint8_t)(addr>>8)); 
  at45_rw((uint8_t)addr); 
  //at45_rw(0x00); 
  while(count--) at45_rw(src[i++]); 
  at45_deselect();    

} 

