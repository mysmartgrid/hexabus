#ifndef LED_BOARD_H_
#define LED_BOARD_H_

#include <process.h>

PROCESS_NAME(led_board_process);

struct pca9532_message {
	uint8_t subaddress;
	uint8_t prescale0;
	uint8_t pwm0;
	uint8_t prescale1;
	uint8_t pwm1;
	uint8_t LEDSelector0;
	uint8_t LEDSelector1;
	uint8_t LEDSelector2;
	uint8_t LEDSelector3;
} __attribute__ ((packed));

#endif
