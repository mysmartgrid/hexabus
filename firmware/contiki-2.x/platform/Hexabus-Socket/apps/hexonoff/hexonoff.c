#include "hexonoff.h"
#include <util/delay.h>
#include "contiki.h"
#include "hexabus_config.h"
#include "endpoints.h"
#include "endpoint_registry.h"

#if HEXONOFF_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint8_t output_vector;


static void set_outputs(uint8_t o_vec) {
    HEXONOFF_PORT = ((HEXONOFF_PORT & ~(output_vector)) | (o_vec & output_vector));
    PRINTF("Setting outputs to %d => %d\n", o_vec, HEXONOFF_PORT);
}

static void toggle_outputs(uint8_t o_vec) {
    HEXONOFF_PORT = ((HEXONOFF_PORT & ~(output_vector)) | (~(HEXONOFF_PORT) & output_vector & o_vec));
    PRINTF("Toggling outputs %d => %d\n", o_vec,HEXONOFF_PORT);
}

static uint8_t get_outputs() {
    return HEXONOFF_PORT & output_vector;
}

static enum hxb_error_code read(struct hxb_value* value)
{
	value->v_u8 = get_outputs();
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_set(const struct hxb_envelope* env)
{
	set_outputs(env->value.v_u8);
	return HXB_ERR_SUCCESS;
}

static const char ep_set_name[] PROGMEM = "Hexonoff, your friendly output setter.";
ENDPOINT_DESCRIPTOR endpoint_hexonoff_set = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_HEXONOFF_SET,
	.name = ep_set_name,
	.read = read,
	.write = write_set
};

static enum hxb_error_code write_toggle(const struct hxb_envelope* env)
{
	toggle_outputs(env->value.v_u8);
	return HXB_ERR_SUCCESS;
}

static const char ep_toggle_name[] PROGMEM = "Hexonoff, your friendly output toggler.";
ENDPOINT_DESCRIPTOR endpoint_hexonoff_toggle = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_HEXONOFF_TOGGLE,
	.name = ep_toggle_name,
	.read = read,
	.write = write_toggle
};

void hexonoff_init(void) {
    PRINTF("Hexonoff init\n");

    output_vector = 0;

    #if defined(HEXONOFF_OUT0)
    output_vector |= (1<<HEXONOFF_OUT0);
    #endif
    #if defined(HEXONOFF_OUT1)
    output_vector |= (1<<HEXONOFF_OUT1);
    #endif
    #if defined(HEXONOFF_OUT2)
    output_vector |= (1<<HEXONOFF_OUT2);
    #endif
    #if defined(HEXONOFF_OUT3)
    output_vector |= (1<<HEXONOFF_OUT3);
    #endif
    #if defined(HEXONOFF_OUT4)
    output_vector |= (1<<HEXONOFF_OUT4);
    #endif
    #if defined(HEXONOFF_OUT5)
    output_vector |= (1<<HEXONOFF_OUT5);
    #endif
    #if defined(HEXONOFF_OUT6)
    output_vector |= (1<<HEXONOFF_OUT6);
    #endif
    #if defined(HEXONOFF_OUT7)
    output_vector |= (1<<HEXONOFF_OUT7);
    #endif

    HEXONOFF_DDR |= output_vector;
    HEXONOFF_PORT &= ~output_vector;
    set_outputs(HEXONOFF_INITIAL_VALUE);

	ENDPOINT_REGISTER(endpoint_hexonoff_set);
	ENDPOINT_REGISTER(endpoint_hexonoff_toggle);
}
