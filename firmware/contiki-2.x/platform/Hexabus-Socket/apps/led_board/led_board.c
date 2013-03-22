#include "led_board.h"

#include "contiki.h"
#include "hexabus_config.h"
#include "endpoint_registry.h"
#include "endpoints.h"
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
	i2c_write_bytes(PCA9532_ADDR, (uint8_t*)&pca_state, sizeof(pca_state));
	
	// TODO what do we do when the write fails?
}

void pca9532_init() {
	pca_state.subaddress = PCA9532_SUBADDRESS;
	pca_state.prescale0 = 0x00;		// frequency (0x00 = fastest)
	pca_state.pwm0 = 0x55; 				// 33% duty cycle
	pca_state.prescale1 = 0x00;
	pca_state.pwm1 = 0xAA; 				// 66% duty cycle
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

void leds_setprescale(uint8_t prescale0, uint8_t prescale1)
{
	pca_state.prescale0 = prescale0;
	pca_state.prescale1 = prescale1;

	pca9532_update();
}

static uint8_t current_color = 0;

static enum hxb_error_code read_led_color(struct hxb_value* val)
{
	val->v_u8 = current_color;
	return HXB_ERR_SUCCESS;
}

// transforms the "numeric" color values (00, 01, 10, 11) into LED Selector bits (00, 10, 11, 01)
uint8_t transform_color(uint8_t val)
{
	switch(val)
	{
		case 0x00: return 0x00;
		case 0x01: return 0x02;
		case 0x02: return 0x03;
		case 0x03: return 0x01;
		default:   return 0x00;
	}
}

static enum hxb_error_code set_led_color(const struct hxb_envelope* env)
{
	uint8_t color = env->value.v_u8;

	uint8_t red = (color & 0x30) >> 4;
	uint8_t green = (color & 0x0C) >> 2;
	uint8_t blue = (color & 0x03);

	red = transform_color(red);
	green = transform_color(green);
	blue = transform_color(blue);

	// Specifically set for the "Board A" version (first Hexahorst/LED-Board revision)
	// LED Selector-to-LED-Mapping:
	// LS0: R|R|R|B
	// LS1: B|G|G|G
	// LS2: -|-|-|B

	uint8_t ls0 = (red << 6)  | (red << 4)   | (red << 2)   | (blue);
	uint8_t ls1 = (blue << 6) | (green << 4) | (green << 2) | (green);
	uint8_t ls2 =                                              blue;
	uint8_t ls3 = 0;

	// blinking (modify PWM prescaler)
	uint8_t prescale0 = (color & 0x40) ? 0x90 : 0x00;
	uint8_t prescale1 = (color & 0x80) ? 0x60 : 0x00;

	PRINTF("PWM0: %d, PWM1: %d\n", prescale0, prescale1);

	leds_setprescale(prescale0, prescale1);
	leds_setselector(ls0, ls1, ls2, ls3);

	return HXB_ERR_SUCCESS;
}

static const char ep_led_color[] PROGMEM = "Hexagl0w LED Color";
ENDPOINT_DESCRIPTOR endpoint_led_color = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_LED_COLOR,
	.name = ep_led_color,
	.read = read_led_color,
	.write = set_led_color
};

PROCESS(led_board_process, "LED Board Controller process");
PROCESS_THREAD(led_board_process, ev, data) {
  PROCESS_BEGIN();
	pca9532_init();

	// register endpoint
	ENDPOINT_REGISTER(endpoint_led_color);

	PROCESS_END();
}
