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
#define SEL1 PB0
// SCLK bit
#define SCLK PB1
// MOSI bit
#define MOSI PB2
// MISO bit
#define MISO PB3
#define SLP_TR PB4

#define AT_RST PB5

// SLP_TR Low/High
#define SLP_TR_LOW  (SPI &= ~(1<<SLP_TR))
#define SLP_TR_HIGH (SPI |=(1<<SLP_TR))

// AT_RST Low/High
#define RST_LOW  (SPI &= ~(1<<AT_RST))
#define RST_HIGH (SPI |= (1<<AT_RST))

#define TRX_IRQ_TRX_END    _BV(3)

/** This macro will protect the following code from interrupts.*/
#define AVR_ENTER_CRITICAL_REGION( ) {uint8_t volatile saved_sreg = SREG; cli( )

/** This macro must always be used in conjunction with AVR_ENTER_CRITICAL_REGION
    so that interrupts are enabled again.*/
#define AVR_LEAVE_CRITICAL_REGION( ) SREG = saved_sreg;}

void spi_masterInit(void);
void spi_regWrite(uint8_t addr, uint8_t data);

#endif /* SPI_H_ */
