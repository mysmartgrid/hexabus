/*
 * button.h
 *
 *  Created on: 18.04.2011
 *      Author: Günter Hildebrandt
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "process.h"

#if defined(__AVR_ATmega1284P__)
#define BUTTON_PORT		PORTD
#define BUTTON_PIN		PIND
#define BUTTON_BIT		PD5
#elif defined(__AVR_ATmega2561__)
#define BUTTON_PORT		PORTE
#define BUTTON_PIN		PINE
#define BUTTON_BIT		PE4
#else
#error Hardware not defined!
#endif

#define DEBOUNCE_TIME		   50
#define DOUBLE_CLICK_TIME	 500UL
#define	CLICK_TIME			2000UL
#define	LONG_CLICK_TIME		7000UL
#define	PAUSE_TIME			 500UL

#define DOUBLE_CLICK_ENABLED	0

PROCESS_NAME(button_pressed_process);


#endif /* BUTTON_H_ */
