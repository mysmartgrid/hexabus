#include "sm_upload.h"

#include "endpoints.h"
#include "endpoint_registry.h"

#include "udp_handler.h"
#include "state_machine.h"
#include "value_broadcast.h"

#include <stdlib.h>

#define LOG_LEVEL SM_UPLOAD_DEBUG
#include "syslog.h"

static enum hxb_error_code read_control(struct hxb_value* value)
{
	syslog(LOG_DEBUG, "READ on SM_CONTROL EP occurred");
	if (sm_is_running()) {
		value->v_u8 = STM_STATE_RUNNING;
	} else {
		value->v_u8 = STM_STATE_STOPPED;
	}
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_control(const struct hxb_envelope* env)
{
	if (env->value.v_u8 == STM_STATE_STOPPED) {
		sm_stop();
	} else if (env->value.v_u8 == STM_STATE_RUNNING) {
		if (sm_is_running()) {
			sm_restart();
		} else {
			sm_start();
		}
	} else {
		return HXB_ERR_INVALID_VALUE;
	}
	return HXB_ERR_SUCCESS;
}

static const char ep_control_name[] PROGMEM = "Statemachine Control";
ENDPOINT_DESCRIPTOR endpoint_sm_upload_control = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_SM_CONTROL,
	.name = ep_control_name,
	.read = read_control,
	.write = write_control
};

static enum hxb_error_code read_receiver(struct hxb_value* value)
{
	syslog(LOG_DEBUG, "READ on SM_UP_RECEIVER EP occurred");
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_receiver(const struct hxb_envelope* env)
{
	char* payload = env->value.v_binary;
	uint8_t chunk_id = (uint8_t) payload[0];
	syslog(LOG_DEBUG, "SM: Attempting to write new chunk %i to EEPROM", chunk_id);
	bool result = sm_write_chunk(chunk_id, payload + 1);

	return result
		? HXB_ERR_SUCCESS
		: HXB_ERR_INVALID_VALUE;
}

static const char ep_receiver_name[] PROGMEM = "Statemachine Upload Receiver";
ENDPOINT_DESCRIPTOR endpoint_sm_upload_receiver = {
	.datatype = HXB_DTYPE_65BYTES,
	.eid = EP_SM_UP_RECEIVER,
	.name = ep_receiver_name,
	.read = read_receiver,
	.write = write_receiver
};

static enum hxb_error_code read_reset(struct hxb_value* value)
{
	syslog(LOG_DEBUG, "READ on SM_RESET_ID EP occurred");
	sm_get_id(value->v_binary);
	return HXB_ERR_SUCCESS;
}

static const char ep_reset_name[] PROGMEM = "Statemachine emergency-reset endpoint.";
ENDPOINT_DESCRIPTOR endpoint_sm_reset = {
	.datatype = HXB_DTYPE_16BYTES,
	.eid = EP_SM_RESET_ID,
	.name = ep_reset_name,
	.read = read_reset,
	.write = 0
};

void sm_upload_init()
{
	ENDPOINT_REGISTER(endpoint_sm_upload_control);
	ENDPOINT_REGISTER(endpoint_sm_upload_receiver);
	ENDPOINT_REGISTER(endpoint_sm_reset);
}
