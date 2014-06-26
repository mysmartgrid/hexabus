#include <vector>

#include <string.h>

#include "libhexabus/sm/machine.hpp"

uint32_t ftou(float f)
{
	uint32_t u;

	memcpy(&u, &f, 4);
	return u;
}

int main()
{
#define U16(v) ((v >> 8) & 0xFF), ((v >> 0) & 0xFF)
#define U32(v) ((v >> 24) & 0xFF), ((v >> 16) & 0xFF), ((v >> 8) & 0xFF), ((v >> 0) & 0xFF)
#define FLOAT(v) U32(ftou(v))

	using namespace hexabus::sm;
	uint8_t program[] = {
		0,
		U16(0x0005),
		U16(0x0005),
		HSO_LD_U8, 5,
		HSO_LD_U32, U32(2342),
		HSO_LD_U8, 5,
		HSO_OP_MOD,
		HSO_WRITE,
		HSO_JNZ_S, 3,
		HSO_LD_U8, 5,
		HSO_RET_CHANGE,
		HSO_LD_U8, 7,
		HSO_RET_CHANGE
	};

#undef U16
#undef U32
#undef FLOAT

	std::vector<uint8_t> program_vect(program, program + sizeof(program));

	Machine machine(program_vect);

	machine.run_sm(0, 0, 0);

	return 0;
}
