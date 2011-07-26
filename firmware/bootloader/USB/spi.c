/*
 * spi.c
 *
 *  Created on: 01.04.2011
 *      Author: Dennis Wilfert
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <avr/sleep.h>
#include <stdio.h>

#include "spi.h"

/* SPI Initialisation (Datasheet page 198) */
void spi_masterInit(void) {
	DDR_SPI |= (1 << MOSI) | (1 << SCLK) | (1 << SEL1); // MOSI und SCK as Output, Rest as input
	SPI |= (1 << MOSI) | (1 << SCLK);
	DDR_SPI &= ~(1 << MISO);
	SPCR |= (1 << SPE) | (1 << MSTR) | (1 << SPI2X); // Activate SPI, Master, clockrate fck/2
}


/*
 * Write a register of the transceiver over SPI
 */
void spi_regWrite(uint8_t addr, uint8_t data) {

	AVR_ENTER_CRITICAL_REGION( );
	/* Prepare the command byte */
	addr |= 0xC0;

	/* Start SPI transaction by pulling SEL low */
	SPI &= ~(1 << SEL1);

	/* Send the Read command byte */
	SPDR = addr;
	while (!(SPSR & (1 << SPIF)));

	_delay_us(100);

	/* Write the byte in the transceiver data register */
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));

	_delay_us(100);
	/* Stop the SPI transaction by setting SEL high */
	SPI |= (1 << SEL1);
	AVR_LEAVE_CRITICAL_REGION( );
}
