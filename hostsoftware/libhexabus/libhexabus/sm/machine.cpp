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
	write_value_t value;

	switch (val->type) {
	case HXB_DTYPE_BOOL:   value = (bool) val->v_uint; break;
	case HXB_DTYPE_UINT8:  value = (uint8_t) val->v_uint; break;
	case HXB_DTYPE_UINT16: value = (uint16_t) val->v_uint; break;
	case HXB_DTYPE_UINT32: value = (uint32_t) val->v_uint; break;
	case HXB_DTYPE_UINT64: value = (uint64_t) val->v_uint64; break;
	case HXB_DTYPE_SINT8:  value = (int8_t) val->v_sint; break;
	case HXB_DTYPE_SINT16: value = (int16_t) val->v_sint; break;
	case HXB_DTYPE_SINT32: value = (int32_t) val->v_sint; break;
	case HXB_DTYPE_SINT64: value = (int64_t) val->v_sint64; break;
	case HXB_DTYPE_FLOAT:  value = (float) val->v_float; break;
	default: return HXB_ERR_INVALID_WRITE;
	}

	return _on_write(eid, value).get_value_or(0);
}

uint64_t Machine::sm_get_systime()
{
	boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1), boost::posix_time::time_duration(0, 0, 0, 0));
	return (boost::posix_time::second_clock::local_time() - epoch).total_seconds();
}

void Machine::sm_diag_msg(int code, const char* file, int line)
{
	static const char* msgs[] = {
		"success",
		"read crossed memory boundaries",
		"write crossed memory boundaries",
		"invalid opcode",
		"invalid types",
		"division by zero",
		"invalid machine code header",
		"invalid operation",
		"stack error",
		"signed overflow",
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

#include "../../shared/sm_machine.c"

}
}
