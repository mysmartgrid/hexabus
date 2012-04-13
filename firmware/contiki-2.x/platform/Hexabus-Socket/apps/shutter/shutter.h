#ifndef SHUTTER_H_
#define SHUTTER_H_

#include "process.h"

#include "hexabus_config.h"

#if defined(__AVR_ATmega1284P__)
#define SHUTTER_PORT   PORTA
#define SHUTTER_IN     PINA
#define SHUTTER_DDR    DDRA

#define SHUTTER_OUT_UP PA2
#define SHUTTER_OUT_DOWN PA3
#define SHUTTER_ENC1 PCINT4
#define SHUTTER_ENC2 PCINT5

#else
#error Hardware not defined!
#endif

#define ENCODER_TIMEOUT        300UL
#define SHUTTER_MAX_BOUND  30000

#define SHUTTER_DIR_UP 1
#define SHUTTER_DIR_DOWN -1
#define SHUTTER_DIR_STOP 0


void    shutter_init(void);

void    shutter_toggle(uint8_t);

void    shutter_set(uint8_t);

void    shutter_stop(void);

uint8_t     shutter_get_state(void);

PROCESS_NAME(shutter_process);
PROCESS_NAME(shutter_setup_process);

#endif /* SHUTTER_H_ */
