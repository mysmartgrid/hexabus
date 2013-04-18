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

struct led_board_command {
	uint8_t command;
	uint8_t cycle_time_h;
	uint8_t cycle_time_s;
	uint8_t cycle_time_v;
	uint8_t begin_h;
	uint8_t end_h;
	uint8_t begin_s;
	uint8_t end_s;
	uint8_t begin_v;
	uint8_t end_v;
} __attribute__ ((packed));

#endif
