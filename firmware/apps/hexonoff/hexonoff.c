#include "hexonoff.h"
#include <util/delay.h>
#include "contiki.h"
#include "hexabus_config.h"
#include "endpoints.h"
#include "endpoint_registry.h"

#define LOG_LEVEL HEXONOFF_DEBUG
#include "syslog.h"

static uint8_t output_vector;


static void set_outputs(uint8_t o_vec) {
    HEXONOFF_PORT = ((HEXONOFF_PORT & ~(output_vector)) | (o_vec & output_vector));
    syslog(LOG_INFO, "Setting outputs to %d => %d", o_vec, HEXONOFF_PORT);
}

static void toggle_outputs(uint8_t o_vec) {
    HEXONOFF_PORT = ((HEXONOFF_PORT & ~(output_vector)) | (~(HEXONOFF_PORT) & output_vector & o_vec));
    syslog(LOG_INFO, "Toggling outputs %d => %d", o_vec,HEXONOFF_PORT);
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

static enum hxb_error_code write_toggle(const struct hxb_envelope* env)
{
	toggle_outputs(env->value.v_u8);
	return HXB_ERR_SUCCESS;
}

void hexonoff_init(void) {
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

	ENDPOINT_REGISTER(HXB_DTYPE_UINT8, EP_HEXONOFF_SET, "Hexonoff, your friendly output setter.", read, write_set);
	ENDPOINT_REGISTER(HXB_DTYPE_UINT8, EP_HEXONOFF_TOGGLE, "Hexonoff, your friendly output toggler.", read, write_toggle);
}
