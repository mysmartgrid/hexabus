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

/*if no interrupt happens in this time then the old measurement out of date and therefore discarded and set to 0 Watt
by reducing this value the accuracy for high power consumptions could be increased but the minimum measurable power consumption increases
for 80s: P_min = 3W */
#define OUT_OF_DATE_TIME 	80

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
