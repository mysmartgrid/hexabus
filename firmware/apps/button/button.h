#ifndef BUTTON_H_
#define BUTTON_H_

#include "hexabus_config.h"
#include "process.h"

PROCESS_NAME(button_pressed_process);

struct button_set_descriptor_t {
	volatile button_pin_t* port;
	button_pin_t mask;

	uint8_t activeLevel:1;

	// NOTE: tick quantities in this structure are guaranteed to hold at 4096 ticks. Larger
	// values may be clipped to 4096
	
	// transitions idle -> active -> idle that fit into this time frame are clicks
	uint16_t click_ticks;
	// transitions idle -> active -> idle report button state after this many ticks, once per tick
	uint16_t pressed_ticks;

	void (*clicked)(button_pin_t button);
	void (*pressed)(button_pin_t button, button_pin_t released, uint16_t pressed_ticks);
};

#define BUTTON_DESCRIPTOR const struct button_set_descriptor_t RODATA

struct button_sm_state_t {
	const struct button_set_descriptor_t* descriptor;
	struct button_sm_state_t* next;

	uint16_t* state;
};

extern struct button_sm_state_t* _button_chain;


#define BUTTON_REGISTER(DESC, pincount) \
	do { \
		static uint16_t state[pincount] = { 0 }; \
		static struct button_sm_state_t _button_state = { 0, 0, state }; \
		_button_state.descriptor = &DESC; \
		_button_state.next = _button_chain; \
		_button_chain = &_button_state; \
	} while (0)

#endif /* BUTTON_H_ */
