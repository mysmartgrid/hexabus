#ifndef HEXASENSE_H_
#define HEXASENSE_H_

#include <stdint.h>
#include "process.h"

#define HEXASENSE_LED1_PORT PORTC
#define HEXASENSE_LED1_DDR  DDRC
#define HEXASENSE_LED1_PIN  6
#define HEXASENSE_LED2_PORT PORTC
#define HEXASENSE_LED2_DDR  DDRC
#define HEXASENSE_LED2_PIN  7
#define HEXASENSE_LED3_PORT PORTA
#define HEXASENSE_LED3_DDR  DDRA
#define HEXASENSE_LED3_PIN  3

#define HEXASENSE_BUTTON1_PORT PINB
#define HEXASENSE_BUTTON1_DDR  DDRB
#define HEXASENSE_BUTTON1_PIN  1
#define HEXASENSE_BUTTON2_PORT PINA
#define HEXASENSE_BUTTON2_DDR  DDRA
#define HEXASENSE_BUTTON2_PIN  2

PROCESS_NAME(hexasense_feedback_process);

void hexasense_init(void);
void toggle_led3();

#endif
