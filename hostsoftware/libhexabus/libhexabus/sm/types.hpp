#ifndef LIBHEXABUS_SM_TYPES_HPP_
#define LIBHEXABUS_SM_TYPES_HPP_

#include <stdint.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/variant.hpp>

namespace hexabus {
namespace sm {

typedef struct {
	uint8_t type;

	bool v_bool;
	uint32_t v_uint;
	uint64_t v_uint64;
	float v_float;
	const char* v_binary;
} hxb_sm_value_t;



#include "sm/sm_types.h"

}
}

#endif
