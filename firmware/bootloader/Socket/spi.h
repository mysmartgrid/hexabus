/*
 * spi.h
 *
 *  Created on: 01.04.2011
 *      Author: Dennis Wilfert
 */

#ifndef SPI_H_
#define SPI_H_
/*
 * Port definitions
 */

// SPI Port
#define SPI PORTB
#define DDR_SPI DDRB

// Sel1 bit
#define SEL1 PB4
// SCLK bit
#define SCLK PB7
// MOSI bit
#define MOSI PB5
// MISO bit
#define MISO PB6

#define SLPAT_PORT PORTD
#define DDR_SLPAT DDRD
#define SLP_TR PD7
#define AT_RST PD6

#define DDR_IRQ DDRD
#define IRQ	PD2


// CPU-Frequency 8Mhz
#define F_CPU           8000000UL							// Takt


// SLP_TR Low/High
#define SLP_TR_LOW  (SLPAT_PORT &= ~(1<<SLP_TR))
#define SLP_TR_HIGH (SLPAT_PORT |=(1<<SLP_TR))

// AT_RST Low/High
#define RST_LOW  (SLPAT_PORT &= ~(1<<AT_RST))
#define RST_HIGH (SLPAT_PORT |= (1<<AT_RST))

#define TRX_IRQ_TRX_END    _BV(3)

/** This macro will protect the following code from interrupts.*/
#define AVR_ENTER_CRITICAL_REGION( ) {uint8_t volatile saved_sreg = SREG; cli( )

/** This macro must always be used in conjunction with AVR_ENTER_CRITICAL_REGION
    so that interrupts are enabled again.*/
#define AVR_LEAVE_CRITICAL_REGION( ) SREG = saved_sreg;}

void spi_masterInit(void);
void spi_regWrite(uint8_t addr, uint8_t data);
uint8_t spi_regRead(uint8_t addr);
void irq_init(void);
int spi_readFrame(uint8_t length, uint8_t *data);
void spi_writeFrame(uint8_t length, uint8_t *data);

volatile uint8_t spi_pktReceived;

#endif /* SPI_H_ */
