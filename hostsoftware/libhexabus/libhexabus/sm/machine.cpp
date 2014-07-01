#include "sm/machine.hpp"

#include "../../../shared/hexabus_types.h"

#include <endian.h>
#include <string.h>

#include <iostream>

namespace hexabus {
namespace sm {

int Machine::sm_get_block(uint16_t at, uint8_t size, void* block)
{
	if (at + size > _program.size())
		return -HSE_OOB_READ;

	memcpy(block, &_program[at], size);

	return size;
}

int Machine::sm_get_u8(uint16_t at, uint32_t* u)
{
	uint8_t buf;

	if (sm_get_block(at, 1, &buf) < 0)
		return -HSE_OOB_READ;

	*u = buf;
	return 1;
}

int Machine::sm_get_u16(uint16_t at, uint32_t* u)
{
	uint16_t buf;

	if (sm_get_block(at, 2, &buf) < 0)
		return -HSE_OOB_READ;

	*u = be16toh(buf);
	return 2;
}

int Machine::sm_get_u32(uint16_t at, uint32_t* u)
{
	uint32_t buf;

	if (sm_get_block(at, 4, &buf) < 0)
		return -HSE_OOB_READ;

	*u = be32toh(buf);
	return 4;
}

int Machine::sm_get_float(uint16_t at, float* f)
{
	uint32_t u;

	if (sm_get_u32(at, &u) < 0)
		return -HSE_OOB_READ;

	memcpy(f, &u, 4);
	return 4;
}

uint8_t Machine::sm_endpoint_write(uint32_t eid, const hxb_sm_value_t* val)
{
	boost::variant<bool, uint8_t, uint32_t, float> value;

	switch (val->type) {
	case HXB_DTYPE_BOOL:   value = (bool) val->v_uint; break;
	case HXB_DTYPE_UINT8:  value = (uint8_t) val->v_uint; break;
	case HXB_DTYPE_UINT32: value = (uint32_t) val->v_uint; break;
	case HXB_DTYPE_FLOAT:  value = (float) val->v_float; break;
	default: return 1;
	}

	return _on_write(eid, value).get_value_or(0);
}

uint32_t Machine::sm_get_timestamp()
{
	return (sm_get_systime() - _created_at).total_seconds();
}

hxb_datetime_t Machine::sm_get_systime()
{
	return boost::posix_time::second_clock::local_time();
}

void Machine::sm_diag_msg(int code, const char* file, int line)
{
	static const char* msgs[] = {
		"success",
		"onvalid read in opcode stream",
		"invalid opcode",
		"invalid types",
		"division by zero",
		"invalid machine code header",
		"invalid operation",
		"stack error",
	};

	std::cerr << "(" << file << ":" << line << "): ";

	if (code < 0 || (size_t) code >= sizeof(msgs) / sizeof(*msgs)) {
		std::cerr << "unknown error " << code;
	} else {
		std::cerr << msgs[code];
	}

	std::cerr << std::endl;
}



#define SM_EXPORT(name) Machine::name

#include "../../shared/sm/machine.c"

}
}
