#include "button.h"

#include "sys/clock.h"
#include "sys/etimer.h"
#include "contiki.h"

PROCESS(button_pressed_process, "Button handler process");

struct button_sm_state_t* _button_chain = 0;

enum button_state_t {
	IDLE = 0,
	DEBOUNCE_ACTIVE = 1,
	ACTIVE = 2,
	DEBOUNCE_IDLE = 3,

	STATE_MASK = 3,
	STATE_SHAMT = 2
};

static void button_iterate_once(void)
{
	for (const struct button_sm_state_t* state = _button_chain; state; state = state->next) {
		uint8_t index = 0;
		struct button_set_descriptor_t d = {};
		memcpy_from_rodata(&d, state->descriptor, sizeof(d));
		for (button_pin_t bit = MAX_BUTTON_BIT; bit; bit >>= 1) {
			if (d.mask & bit) {
				button_pin_t level = *d.port & bit;
				bool activeLevel = (level != 0 && d.activeLevel != 0) || (level == 0 && d.activeLevel == 0);

				uint16_t ticks = (state->state[index] >> STATE_SHAMT);
				if (((ticks + BUTTON_DEBOUNCE_TICKS) & (0xFFFF >> STATE_SHAMT)) > ticks) {
					ticks += BUTTON_DEBOUNCE_TICKS;
				} else {
					ticks = 0xFFFF >> STATE_SHAMT;
				}
				uint16_t smState = state->state[index] & STATE_MASK;
				switch (smState) {
					case IDLE:
						if (activeLevel) {
							smState = DEBOUNCE_ACTIVE;
							ticks = 0;
						}
						break;

					case DEBOUNCE_ACTIVE:
					case ACTIVE:
						if (activeLevel) {
							smState = ACTIVE;
							if (d.pressed && d.pressed_ticks <= ticks) {
								d.pressed(bit, 0, ticks);
							}
						} else if (smState == DEBOUNCE_ACTIVE) {
							smState = IDLE;
							ticks = 0;
						} else if (smState == ACTIVE) {
							smState = DEBOUNCE_IDLE;
						}
						break;

					case DEBOUNCE_IDLE:
						if (activeLevel) {
							smState = ACTIVE;
						} else {
							if (d.clicked && d.click_ticks >= ticks) {
								d.clicked(bit);
							}
							if (d.pressed && d.pressed_ticks <= ticks) {
								d.pressed(bit, 1, ticks);
							}
							smState = IDLE;
							ticks = 0;
						}
						break;
				}
				state->state[index] = (ticks << 2) | smState;
				index++;
			}
		}
	}
}

PROCESS_THREAD(button_pressed_process, ev, data)
{
	PROCESS_EXITHANDLER(goto exit);

	static struct etimer button_timer;

	PROCESS_BEGIN();

	while (1) {
		button_iterate_once();
		etimer_set(&button_timer, BUTTON_DEBOUNCE_TICKS);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
	}

exit: ;

	PROCESS_END();
}

