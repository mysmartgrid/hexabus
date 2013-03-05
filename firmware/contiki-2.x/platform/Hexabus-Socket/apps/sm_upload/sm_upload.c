#include "sm_upload.h"

#include "endpoints.h"
#include "endpoint_registry.h"

#include "udp_handler.h"
#include "state_machine.h"
#include "value_broadcast.h"
#include "packet_builder.h"

#include <stdlib.h>

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static enum hxb_error_code read_control(struct hxb_value* value)
{
	PRINTF("READ on SM_CONTROL EP occurred\n");
	if (sm_is_running()) {
		value->v_u8 = STM_STATE_RUNNING;
	} else {
		value->v_u8 = STM_STATE_STOPPED;
	}
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_control(const struct hxb_value* value)
{
	if (value->v_u8 == STM_STATE_STOPPED) {
		sm_stop();
	} else if (value->v_u8 == STM_STATE_RUNNING) {
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
	PRINTF("READ on SM_UP_RECEIVER EP occurred\n");
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_receiver(const struct hxb_value* value)
{
	PRINTF("SM: Attempting to write new chunk to EEPROM\n");
	struct hxb_packet_u8 result = {
		.type = HXB_PTYPE_INFO,
		.datatype = HXB_DTYPE_BOOL,
		.eid = EP_SM_UP_ACKNAK
	};
	if (value->datatype == HXB_DTYPE_66BYTES) {
		char* payload = value->v_binary;
		uint8_t chunk_id = (uint8_t) payload[0];
		result.value = sm_write_chunk(chunk_id, payload + 1);
		packet_finalize_u8(&result);
		PRINTF(result.value ? "SENDING ACK" : "SENDING NACK");
		send_packet(&result, sizeof(result));
		return result.value
			? HXB_ERR_SUCCESS
			: HXB_ERR_INVALID_VALUE;
	} else {
		// send NAK to SM_SENDER
		PRINTF("SENDING ERROR DATATYPE");
		result.value = HXB_FALSE;
		packet_finalize_u8(&result);
		send_packet(&result, sizeof(result));
		return HXB_ERR_DATATYPE;
	}
}

static const char ep_receiver_name[] PROGMEM = "Statemachine Upload Receiver";
ENDPOINT_DESCRIPTOR endpoint_sm_upload_receiver = {
	.datatype = HXB_DTYPE_66BYTES,
	.eid = EP_SM_UP_RECEIVER,
	.name = ep_receiver_name,
	.read = read_receiver,
	.write = write_receiver
};

static enum hxb_error_code read_acknack(struct hxb_value* value)
{
	PRINTF("READ on SM_UP_ACKNAK EP occurred\n");
	return HXB_ERR_SUCCESS;
}

static const char ep_acknack_name[] PROGMEM = "Statemachine Upload ACK/NAK";
ENDPOINT_DESCRIPTOR endpoint_sm_upload_acknack = {
	.datatype = HXB_DTYPE_BOOL,
	.eid = EP_SM_UP_ACKNAK,
	.name = ep_acknack_name,
	.read = read_acknack,
	.write = 0
};

static enum hxb_error_code read_reset(struct hxb_value* value)
{
	PRINTF("READ on SM_RESET_ID EP occurred\n");
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
	ENDPOINT_REGISTER(endpoint_sm_upload_acknack);
	ENDPOINT_REGISTER(endpoint_sm_reset);
}
