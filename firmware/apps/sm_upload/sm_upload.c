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

static enum hxb_error_code read_acknack(struct hxb_value* value)
{
	syslog(LOG_DEBUG, "READ on SM_UP_ACKNAK EP occurred");
	return HXB_ERR_SUCCESS;
}

void sm_upload_init()
{
	ENDPOINT_REGISTER(HXB_DTYPE_UINT8, EP_SM_CONTROL, "Statemachine Control", read_control, write_control);
	ENDPOINT_REGISTER(HXB_DTYPE_65BYTES, EP_SM_UP_RECEIVER, "Statemachine Upload Receiver", read_receiver, write_receiver);
}
