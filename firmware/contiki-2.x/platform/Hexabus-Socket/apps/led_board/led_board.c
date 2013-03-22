#include "led_board.h"

#include "contiki.h"
#include "hexabus_config.h"
#include "i2cmaster.h"
#include "i2c.h"

#include <util/delay.h>

#if LED_BOARD_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// CONFIG FOR THE LED DRIVER
#define PCA9532_ADDR  0xC0
// TODO put this into hexabus_config.h

// CONSTANTS FOR THE LED DRIVER
#define PCA9532_SUBADDRESS 0x12
#define PCA9532_FULL 0x55
#define PCA9532_OFF 0x00

struct pca9532_message pca_state;

// pushes the current pca_state to the PCA9532
// so other functions can just change the part of the state they want to change and then call update to push the whole thing.
void pca9532_update() {
	printf("pca9532_update\n");
	i2c_write_bytes(PCA9532_ADDR, &pca_state, sizeof(pca_state));
	
	// TODO what do we do when the write fails?
}

void pca9532_init() {
	printf("pca9532_init\n");

	pca_state.subaddress = PCA9532_SUBADDRESS;
	pca_state.prescale0 = 0x00;		// maximum frequency
	pca_state.pwm0 = 0xFF; 				// 100% duty cycle
	pca_state.prescale1 = 0x00;
	pca_state.pwm1 = 0xFF;
	pca_state.LEDSelector0 = PCA9532_OFF;
	pca_state.LEDSelector1 = PCA9532_OFF;
	pca_state.LEDSelector2 = PCA9532_OFF;
	pca_state.LEDSelector3 = PCA9532_OFF;

	pca9532_update();
}

void leds_setselector(uint8_t sel0, uint8_t sel1, uint8_t sel2, uint8_t sel3) {
	pca_state.LEDSelector0 = sel0;
	pca_state.LEDSelector1 = sel1;
	pca_state.LEDSelector2 = sel2;
	pca_state.LEDSelector3 = sel3;

	pca9532_update();
}

void leds_setPWM(uint8_t pwm0, uint8_t pwm1)
{
	pca_state.pwm0 = pwm0;
	pca_state.pwm1 = pwm1;

	pca9532_update();
}

// making the leds do spiffy things
uint8_t selectors[] = {
	PCA9532_OFF, PCA9532_OFF, PCA9532_OFF, PCA9532_OFF,
	PCA9532_FULL, PCA9532_OFF, PCA9532_OFF, PCA9532_OFF,
	PCA9532_FULL, PCA9532_FULL, PCA9532_OFF, PCA9532_OFF,
	PCA9532_FULL, PCA9532_FULL, PCA9532_FULL, PCA9532_OFF,
	PCA9532_FULL, PCA9532_FULL, PCA9532_FULL, PCA9532_FULL,
	PCA9532_FULL, PCA9532_FULL, PCA9532_FULL, PCA9532_OFF,
	PCA9532_FULL, PCA9532_FULL, PCA9532_OFF, PCA9532_OFF,
	PCA9532_FULL, PCA9532_OFF, PCA9532_OFF, PCA9532_OFF,
	PCA9532_OFF
};
uint8_t brightness[] = {
	0xF0, 0xE0, 0xD0, 0xC0, 0xB0,
	0xA0, 0x90, 0x80, 0x70, 0x60,
	0x50, 0x40, 0x30, 0x20, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00
};
uint8_t sel_index, brightness_index, time_step;
void animation_step() { // TODO of course this is just for testing - we need to be more flexible than this.
	if(!(time_step++ % 64)) {
		sel_index = sel_index++ % 33;
		leds_setselector(
			selectors[sel_index], selectors[(sel_index + 1) % 33],
			selectors[(sel_index + 2) % 33], selectors[(sel_index + 3) % 33]);
	}

	brightness_index = brightness_index++ % 200;
	leds_setPWM(brightness[brightness_index % 20], brightness[(brightness_index + 1) % 20]);
}
// ~~~~~~ ~~~ ~~~~ ~~ ~~~~~~ ~~~~~~


PROCESS(led_board_process, "LED Board Controller process");
PROCESS_THREAD(led_board_process, ev, data) {
	printf("PCA9532 control process starting...\n");

  PROCESS_BEGIN();
	pca9532_init();

  static struct etimer periodic;
  etimer_set(&periodic, 0.02*CLOCK_SECOND); // TODO make interval configurable
  while(1) {
		// sleep for a bit.
    PROCESS_YIELD();
    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);

			animation_step();
    }
  }

	PROCESS_END();
}
