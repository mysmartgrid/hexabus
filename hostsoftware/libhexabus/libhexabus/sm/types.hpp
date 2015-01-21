#ifndef LIBHEXABUS_SM_TYPES_HPP_
#define LIBHEXABUS_SM_TYPES_HPP_

#include <stdint.h>

namespace hexabus {
namespace sm {

typedef struct {
	uint8_t type;

	uint32_t v_uint;
	uint64_t v_uint64;
	int32_t v_sint;
	int64_t v_sint64;
	float v_float;
	const char* v_binary;
} hxb_sm_value_t;



#include "sm/sm_types.h"

}
}

#endif
