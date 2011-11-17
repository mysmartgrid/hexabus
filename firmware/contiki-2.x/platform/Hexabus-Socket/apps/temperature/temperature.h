/*
 * temperature.h
 *
 *  Created on: 20.04.2011
 *      Author: Mathias Dalheimer
 */

#ifndef TEMPERATURE_H
#define TEMPERATURE_H 1



#include <stdint.h>
#include <stdbool.h>
#include "process.h"


PROCESS_NAME(temperature_process);
#define METERING_VECT 		INT1_vect //Interrupt for the Metering


#define SET_METERING_INT( ) ( EICRA |= ((1 << ISC11) | (1 << ISC10)) ) // rising edge on INT1 generates interrupt

#define ENABLE_METERING_INTERRUPT( ) 			( EIMSK |= ( 1 << INT1 ) )
#define DISABLE_METERING_INTERRUPT( ) 			( EIMSK &= ~( 1 << INT1 ) )


/** \brief This function returns the measured electrical power in Watt.
 *
 **/
float 	temperature_get(void);
char* temperature_as_string(void);
void 	temperature_init(void);
void	temperature_start(void);
void	temperature_stop(void);
void	temperature_reset(void);

#endif /* TEMPERATURE_H */
