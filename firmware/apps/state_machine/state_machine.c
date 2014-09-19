#include "contiki.h"

#include "hexabus_config.h"
#include "value_broadcast.h"
#include "udp_handler.h"
#include "endpoints.h"
#include "state_machine.h"
#include "endpoint_registry.h"
#include "datetime_service.h"
#include "sm_types.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "nvm.h"

#define LOG_LEVEL STATE_MACHINE_DEBUG
#include "syslog.h"

/*------------------------------------------------------*/
PROCESS_NAME(state_machine_process);
PROCESS(state_machine_process, "State Machine Process");
AUTOSTART_PROCESSES(&state_machine_process);
/*------------------------------------------------------*/

static process_event_t sm_handle_input_event;

enum {
	SM_STACK_SIZE = 32,
	SM_MEMORY_SIZE = 128,
};

static uint8_t sm_memory[SM_MEMORY_SIZE];
static hxb_sm_value_t sm_stack[SM_STACK_SIZE];
static bool sm_first_run;

static uint32_t sm_get_timestamp();

/**
 * Functions to start/stop the state machine itself.
 */

uint8_t sm_is_running()
{
	return (process_is_running(&state_machine_process));
}

void sm_start()
{
	if (process_is_running(&state_machine_process)) {
		syslog(LOG_DEBUG, "State machine process is already running - not starting it.");
	} else {
		syslog(LOG_DEBUG, "Starting state machine process.");
		sm_first_run = true;
		process_start(&state_machine_process, NULL);
	}
}

void sm_stop()
{
	if (!process_is_running(&state_machine_process)) {
		syslog(LOG_DEBUG, "State machine process is already stopped - not stopping it.");
	} else {
		syslog(LOG_DEBUG, "Stopping state machine process.");
		process_exit(&state_machine_process);
	}
}

void sm_restart()
{
	syslog(LOG_DEBUG, "Attempting to restart state machine process.");
	sm_stop();
	sm_start();
	syslog(LOG_DEBUG, "Restart complete");
}


static uint8_t sm_endpoint_write(uint32_t eid, const hxb_sm_value_t* val)
{
	struct hxb_envelope env;

	env.value.datatype = val->type;
	switch (val->type) {
	case HXB_DTYPE_BOOL:
		env.value.v_bool = val->v_uint;
		break;

	case HXB_DTYPE_UINT8:
		env.value.v_u8 = val->v_uint;
		break;

	case HXB_DTYPE_UINT32:
		env.value.v_u32 = val->v_uint;
		break;

	case HXB_DTYPE_FLOAT:
		env.value.v_float = val->v_float;
		break;

	default:
		return HXB_ERR_INVALID_WRITE;
	}

	syslog(LOG_DEBUG, "write %lu -> %lu", eid, val->v_uint);

	uip_ip6addr_u8(&env.src_ip, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
	env.src_port = 0;
	env.eid = eid;

	return endpoint_write(eid, &env);
}

static uint64_t sm_get_systime()
{
	uint64_t d;

	getDatetime(&d);
	return d;
}

#define STR2(x) #x
#define STR(x) STR2(x)
#define sm_diag_msg(code, file, line) \
	do { \
		syslog(LOG_DEBUG, "(sm:" STR(line) "): %i", code); \
	} while (0)

#define SM_EXPORT(name) name

#include "../../../../../shared/sm_machine.c"


static void sm_udp_reset()
{
	// the udp handler has transitioned to "ready" for some reason, most likely a cold reboot
	// since anything that can bring the udp handler down will also affect the state machine, reset it
	// special case: ID 0 is reserved for state machines that should not be automatically reset
	if (!sm_id_is_zero()) {
		syslog(LOG_NOTICE, "UDP handler reset, broadcasting Reset-ID");
		broadcast_value(EP_SM_RESET_ID);
	}
}

static void sm_handle_periodic()
{
	run_sm(0, 0, 0);
}

void sm_handle_input(const struct hxb_envelope* env)
{
	if (!sm_id_is_zero()) {
		// if our state machine ID is not 0 (0 means "no auto reset".
		// check if it's a Reset-ID (this means another state machine on the network was just unexpectedly reset)
		syslog(LOG_DEBUG, "SM_ID is not zero.");
		if (env->eid == EP_SM_RESET_ID && env->value.datatype == HXB_DTYPE_16BYTES) {
			syslog(LOG_NOTICE, "Received Reset-ID from " LOG_6ADDR_FMT, LOG_6ADDR_VAL(env->src_ip));
			if (sm_id_equals(env->value.v_binary))
				sm_restart();
			return;
		}
	}

	hxb_sm_value_t value;

	value.type = env->value.datatype;
	switch (value.type) {
	case HXB_DTYPE_BOOL:
		value.v_uint = env->value.v_bool;
		break;

	case HXB_DTYPE_UINT8:
		value.v_uint = env->value.v_u8;
		break;

	case HXB_DTYPE_UINT32:
		value.v_uint = env->value.v_u32;
		break;

	case HXB_DTYPE_UINT64:
		value.v_uint64 = env->value.v_u64;
		break;

	case HXB_DTYPE_FLOAT:
		value.v_uint = env->value.v_float;
		break;


	case HXB_DTYPE_128STRING:
	case HXB_DTYPE_65BYTES:
	case HXB_DTYPE_16BYTES:
		return;

	default:
		syslog(LOG_INFO, "can't handle packet data type %i", value.type);
		return;
	}

	run_sm((const char*) env->src_ip.u8, env->eid, &value);

	syslog(LOG_DEBUG, "Now in state: %d", sm_curstate);
}

PROCESS_THREAD(state_machine_process, ev, data)
{
	PROCESS_BEGIN();

	sm_handle_input_event = process_alloc_event();

	static struct etimer check_timer;
	etimer_set(&check_timer, CLOCK_SECOND * 1);

	while (sm_is_running()) {
		PROCESS_WAIT_EVENT();
		if (!sm_is_running())
			break;

		if (ev == udp_handler_event && *(udp_handler_event_t*) data == UDP_HANDLER_UP) {
			sm_udp_reset();
		} else {
			if (ev == PROCESS_EVENT_TIMER && etimer_expired(&check_timer)) {
				syslog(LOG_DEBUG, "periodic, state: %d", sm_curstate);
				sm_handle_periodic();
				etimer_reset(&check_timer);
			} else if (ev == sm_handle_input_event) {
				syslog(LOG_DEBUG, "packet, state: %d", sm_curstate);
				sm_handle_input(data);
			}
		}
	}

	PROCESS_END();
}
