#ifndef APPS_STATE_MACHINE_SM_TYPES_H
#define APPS_STATE_MACHINE_SM_TYPES_H

#include "../../../../../shared/hexabus_types.h"

typedef struct {
	uint8_t type;

	union {
		bool v_bool;
		uint32_t v_uint;
		uint64_t v_uint64;
		float v_float;
		const char* v_binary;
	};
} hxb_sm_value_t;

#include "../../../../../shared/sm_types.h"

#endif
