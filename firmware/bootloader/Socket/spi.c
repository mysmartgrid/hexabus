/*
 * spi.c
 *
 *  Created on: 01.04.2011
 *      Author: Dennis Wilfert
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
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

/*
 * Read a register of the transceiver over SPI
 */
uint8_t spi_regRead(uint8_t addr) {
	uint8_t register_value;

	AVR_ENTER_CRITICAL_REGION( );
	/* Prepare the command byte */
	addr |= 0x80;

	/* Start SPI transaction by pulling SEL low */
	SPI &= ~(1 << SEL1);

	/* Send the Read command byte */
	SPDR = addr;

	while (!(SPSR & (1 << SPIF)));

	/* Do dummy read for initiating SPI read */
	SPDR = 0x00;

	while (!(SPSR & (1 << SPIF)));

	/* Read the byte received */
	register_value = SPDR;

	/* Stop the SPI transaction by setting SEL high */
	SPI |= (1 << SEL1);
	AVR_LEAVE_CRITICAL_REGION( );
	return register_value;
}

/*
 * Initialize the external interrupt from the transceiver
 */
void irq_init(void) {
	DDR_IRQ &= ~(1 << IRQ);
	EIMSK |= (1 << INT0);
	EICRA |= (1 << ISC01) | (1 << ISC00);
}

/*
 * Read a packet from the transceiver
 */
int spi_readFrame(uint8_t length, uint8_t *data) {

	uint8_t fl;
	AVR_ENTER_CRITICAL_REGION( );
	EIMSK &= ~(1 << INT0);

	SPI &= ~(1 << SEL1); // Set SEL low to start SPI

	SPDR = 0x20; // Send the command byte

	while (!(SPSR & (1 << SPIF)));

    uint8_t frame_length = SPDR;

    /*Read frame length.*/
    SPDR = frame_length;
    while ((SPSR & (1 << SPIF)) == 0) {;}
    frame_length = SPDR;
    fl = frame_length;
	uint8_t temp;

	if (length != 1 && frame_length <= length) {

        /*Upload frame buffer to data pointer. Calculate CRC.*/
        SPDR = frame_length;
        while ((SPSR & (1 << SPIF)) == 0) {;}

		do {
			temp = SPDR;
			SPDR = 0x00; // Do dummy write for initiating SPI read
			*data = temp;

			data++;
			while (!(SPSR & (1 << SPIF)));
//			length--;

		} while (--frame_length > 0);
	}

	SPDR = 0;       /*  dummy write */
    //read ED
    while ((SPSR & (1 << SPIF)) == 0) {;}
    temp = SPDR;
    SPDR = 0;       /*  dummy write */
    //read RX_STATUS
    while ((SPSR & (1 << SPIF)) == 0) {;}
    temp = SPDR;
    SPDR = 0;       /*  dummy write */

	SPI |= (1 << SEL1); // Stop SPI
	EIMSK |= (1 << INT0);
	AVR_LEAVE_CRITICAL_REGION( );
	return fl;
}

/*
 * Write a packet to the transceiver and send it
 */
void spi_writeFrame(uint8_t length, uint8_t *data) {

	AVR_ENTER_CRITICAL_REGION( );
	SPI &= ~(1 << SEL1); // Set SEL low to start SPI

	SPDR |= 0x60; // Send Frame write byte

	while (!(SPSR & (1 << SPIF)));

	SPDR = length;

	do {

		while (!(SPSR & (1 << SPIF)));

		SPDR = *data++;

		length--;

	} while (length > 0);

	while (!(SPSR & (1 << SPIF))); // Wait until the last byte is sent

	SPI |= (1 << SEL1); // Set SEL high to stop SPI
	AVR_LEAVE_CRITICAL_REGION( );
}

ISR(INT0_vect)
{
	static volatile uint8_t irq_cause;
	irq_cause = spi_regRead(0x0F);

	if (irq_cause & TRX_IRQ_TRX_END) {
		spi_pktReceived = 1;
	}

}
