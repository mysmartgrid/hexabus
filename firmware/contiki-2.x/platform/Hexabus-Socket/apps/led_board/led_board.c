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

// definitions of LED selectors for the Hexasense beta board
// (the board with the three LED clusters).
#define LS0_RED_OFF  0x00
#define LS0_RED_ON   0x54
#define LS0_RED_PWM0 0xA8
#define LS0_RED_PWM1 0xFA
#define LS1_GREEN_OFF  0x00
#define LS1_GREEN_ON   0x15
#define LS1_GREEN_PWM0 0x2A
#define LS1_GREEN_PWM1 0x3F
// The blue LEDs are distributed to several LED selectors. Therefore
// their LED selector codes have to be OR'ed to the other LED selectors.
#define LS0_BLUE_OFF  0x00
#define LS0_BLUE_ON   0x01
#define LS0_BLUE_PWM0 0x02
#define LS0_BLUE_PWM1 0x03
#define LS1_BLUE_OFF  0x00
#define LS1_BLUE_ON   0x40
#define LS1_BLUE_PWM0 0x80
#define LS1_BLUE_PWM1 0xC0
#define LS2_BLUE_OFF  0x00
#define LS2_BLUE_ON   0x01
#define LS2_BLUE_PWM0 0x02
#define LS2_BLUE_PWM1 0x03
// nothing is attached to LS3
#define LS3 0x00
// ============================================================================
// TODO do this differently! Just send "setselector" the codes (on, off, pwm0/1)
//      for each color -- and let setselector handle all the board specific stuff

void leds_setselector(uint8_t sel0, uint8_t sel1, uint8_t sel2, uint8_t sel3) {
	pca_state.LEDSelector0 = sel0;
	pca_state.LEDSelector1 = sel1;
	pca_state.LEDSelector2 = sel2;
	pca_state.LEDSelector3 = sel3;

	pca9532_update();
}

void leds_setPWM(uint8_t pwm0, uint8_t pwm1) {
	pca_state.pwm0 = pwm0;
	pca_state.pwm1 = pwm1;

	pca9532_update();
}

void leds_setprescale(uint8_t prescale0, uint8_t prescale1) {
	pca_state.prescale0 = prescale0;
	pca_state.prescale1 = prescale1;

	pca9532_update();
}

struct led_board_command current_command;

static enum hxb_error_code read_led_color(struct hxb_value* val) {
	// TODO memset(val->v_binary, 0, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH);
	// TODO memcpy(val->v_binary, current_command, sizeof(current_command));
	return HXB_ERR_SUCCESS;
}

enum hsv_case {	V_ZERO, V_FULL, S_ZERO, S_FULL };
enum hsv_case hsv_simplification_case;

void set_color(uint8_t h, uint8_t s, uint8_t v)
{
	// in two of the cases, we don't have to calculate much:
	if(hsv_simplification_case == V_ZERO || hsv_simplification_case == S_ZERO) {
		if(hsv_simplification_case == V_ZERO) { // if V is zero, that means it's black -- S and H don't matter.
			// lights out!
			leds_setselector(LS0_RED_OFF | LS0_BLUE_OFF, LS1_GREEN_OFF | LS1_BLUE_OFF, LS2_BLUE_OFF, LS3);
		} else {
			// all LEDs to "v"
			leds_setPWM(v,0);
			leds_setselector(LS0_RED_PWM0 | LS0_BLUE_PWM0, LS1_GREEN_PWM0 | LS1_BLUE_PWM0, LS2_BLUE_PWM0, LS3);
		}
	} 
	// here we actually have to calculate something
	else {
		// calculate p, t, and q
		uint8_t f = (uint8_t)((float)h / 42.666666667f) * 6;
		uint8_t p, q, t;
	  if(hsv_simplification_case == S_FULL)
		{
			p = 0;
			q = v * (255 - f);
			t = 0;
		} else { // V_FULL
			p = 255 - s;
			q = 255 - f;
			t = ((255 - (uint16_t)s) * (255 - (uint16_t)f)) / 256;
		}

		// find out which sector we're in. Each 60degree sector of HSV space corresponds to 42+(2/3)/255 in our H-normalized-to-255 space.
		if(h < 43) { // actually 42 2/3
			// r = v, g = t, b = p
			if(hsv_simplification_case == S_FULL) {
				// p = t = 0;
				leds_setPWM(v,0);
				leds_setselector(LS0_RED_PWM0 | LS0_BLUE_OFF, LS1_GREEN_OFF | LS1_BLUE_OFF, LS2_BLUE_OFF, LS3);
			} else { // V_FULL
				leds_setPWM(p,t);
				leds_setselector(LS0_RED_ON | LS0_BLUE_PWM0, LS1_GREEN_PWM1 | LS1_BLUE_PWM0, LS2_BLUE_PWM0, LS3);
			}
		} else if(h < 86) { // actually 85 1/3
			// r = q; g = v; b = p;
			if(hsv_simplification_case == S_FULL) {
				// p = 0;
				leds_setPWM(v,q);
				leds_setselector(LS0_RED_PWM1 | LS0_BLUE_OFF, LS1_GREEN_PWM0 | LS1_BLUE_OFF, LS2_BLUE_OFF, LS3);
			} else { // V_FULL
				// v = 255;
				leds_setPWM(p,q);
				leds_setselector(LS0_RED_PWM1 | LS0_BLUE_PWM0, LS1_GREEN_ON | LS1_BLUE_PWM0, LS2_BLUE_PWM0, LS3);
			}
		} else if(h < 128) { // actually 128
			// r = p; g = v; b = t;
			if(hsv_simplification_case == S_FULL)
			{
				// p = t = 0;
				leds_setPWM(v,0);
				leds_setselector(LS0_RED_OFF | LS0_BLUE_OFF, LS1_GREEN_PWM0 | LS1_BLUE_OFF, LS2_BLUE_OFF, LS3);
			} else { // V_FULL
				// v = 255;
				leds_setPWM(p,t);
				leds_setselector(LS0_RED_PWM0 | LS0_BLUE_PWM1, LS1_GREEN_ON | LS1_BLUE_PWM1, LS2_BLUE_PWM1, LS3);
			}
		} else if(h < 171) { // actually 170 2/3
			// r = p; g = q; b = v;
			if(hsv_simplification_case == S_FULL) {
				// p = t = 0;
				leds_setPWM(v,q);
				leds_setselector(LS0_RED_OFF | LS0_BLUE_PWM0, LS1_GREEN_PWM1 | LS1_BLUE_PWM0, LS2_BLUE_PWM0, LS3);
			} else {
				// v = 255;
				leds_setPWM(p,q);
				leds_setselector(LS0_RED_PWM0 | LS0_BLUE_ON, LS1_GREEN_PWM1 | LS1_BLUE_ON, LS2_BLUE_ON, LS3);
			}
		} else if(h < 214) { // actually 213 1/3
			// r = t; g = p; b = v;
			if(hsv_simplification_case == S_FULL) {
				// p = t = 0;
				leds_setPWM(v,0);
				leds_setselector(LS0_RED_OFF | LS0_BLUE_PWM0, LS1_GREEN_OFF | LS1_BLUE_PWM0, LS2_BLUE_PWM0, LS3);
			} else {
				// v = 255;
				leds_setPWM(p,t);
				leds_setselector(LS0_RED_PWM1 | LS0_BLUE_ON, LS1_GREEN_PWM0 | LS1_BLUE_ON, LS2_BLUE_ON, LS3);
			}
		} else { // 214 .. 255
			// r = v; g = p; b = q;
			if(hsv_simplification_case == S_FULL) {
				// p = t = 0;
				leds_setPWM(v,q);
				leds_setselector(LS0_RED_PWM0 | LS0_BLUE_PWM1, LS1_GREEN_OFF | LS1_BLUE_PWM1, LS2_BLUE_PWM1, LS3);
			} else { // V_FULL
				// v = 255;
				leds_setPWM(p,q);
				leds_setselector(LS0_RED_ON | LS0_BLUE_PWM1, LS1_GREEN_PWM0 | LS1_BLUE_PWM1, LS2_BLUE_PWM1, LS3);
			}
		}
	}
}

void compute_hsv_simplification()
{
	// hsv-simplification -- restrict values: We only have 2 PWM channels.
	// that limits our color space: We set either S or V to be either constant 255 or 0.
	// -> find the channel (s or v) that has its average closest to 255 or 0.
	uint8_t s_avg = (current_command.begin_s / 2) + (current_command.end_s / 2);
	uint8_t v_avg = (current_command.begin_v / 2) + (current_command.end_v / 2);

	// now we look at the distances between s_/v_avg and 0 or 255. Whichever is closest will be our constant.
	if(s_avg > 127) {
		if(v_avg > 127) {
			if(v_avg > s_avg) // v_avg is closest to 255
				hsv_simplification_case = V_FULL;
			else // s_avg is greatest
				hsv_simplification_case = S_FULL;
		} else { // v_avg <= 127
			if((255-v_avg) < s_avg) // s_avg is closest to 255
				hsv_simplification_case = S_FULL;
			else // v_avg is closest to 0
				hsv_simplification_case = V_ZERO;
		}
	} else {
		if(v_avg > 127) {
			if((255 - s_avg) < v_avg) // s_avg is closest to 0
				hsv_simplification_case = S_ZERO;
			else // v_avg is closest to 255.
				hsv_simplification_case = V_FULL;
		} else {
			if(s_avg < v_avg) // s_avg is closest to 0
				hsv_simplification_case = S_ZERO;
			else // v_avg is closest to 0.
				hsv_simplification_case = V_ZERO;
		}
	}

	// TODO set v or s to 0 or 255 depending on case.
}

static enum hxb_error_code set_led_color(const struct hxb_envelope* env)
{
	struct led_board_command* cmd = (struct led_board_command*)env->value.v_binary;

	current_command = *cmd;
	return HXB_ERR_SUCCESS;
}

static const char ep_led_color[] PROGMEM = "Hexagl0w LED Color";
ENDPOINT_DESCRIPTOR endpoint_led_color = {
	.datatype = HXB_DTYPE_16BYTES,
	.eid = EP_LED_COLOR,
	.name = ep_led_color,
	.read = read_led_color,
	.write = set_led_color
};

PROCESS(led_board_process, "LED Board Controller process");
PROCESS_THREAD(led_board_process, ev, data) {
  PROCESS_BEGIN();

	// memset(&current_command, 0, sizeof(current_command)); // clear command

	pca9532_init();

	ENDPOINT_REGISTER(endpoint_led_color);

	PROCESS_END();
}
