/*
 * metering.h
 *
 *  Created on: 20.04.2011
 *      Author: Günter Hildebrandt
 */

#ifndef METERING_H_
#define METERING_H_

#include <stdint.h>
#include <stdbool.h>


#define METERING_VECT 		INT1_vect //Interrupt for the Metering


#define SET_METERING_INT( ) ( EICRA |= ((1 << ISC11) | (1 << ISC10)) ) // rising edge on INT1 generates interrupt

#define ENABLE_METERING_INTERRUPT( ) 			( EIMSK |= ( 1 << INT1 ) )
#define DISABLE_METERING_INTERRUPT( ) 			( EIMSK &= ~( 1 << INT1 ) )


/** \brief This function returns the measured electrical power in Watt.
 *
 **/
uint16_t 	metering_get_power(void);
void 		metering_init(void);
void		metering_start(void);
void		metering_stop(void);
void		metering_reset(void);
bool		metering_calibrate(void);
#endif /* METERING_H_ */
