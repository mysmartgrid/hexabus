/*
 * button.h
 *
 *  Created on: 18.04.2011
 *      Author: Günter Hildebrandt
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "process.h"

#define BUTTON_PORT		PORTE
#define BUTTON_PIN		PINE
#define BUTTON_BIT		PE2

#define DEBOUNCE_TIME		   50
#define DOUBLE_CLICK_TIME	 500UL
#define	CLICK_TIME			2000UL
#define	LONG_CLICK_TIME		5000UL
#define	PAUSE_TIME			 500UL

#define DOUBLE_CLICK_ENABLED	0

PROCESS_NAME(button_pressed_process);


#endif /* BUTTON_H_ */
