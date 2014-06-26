#include <utility>
#include <vector>

#include <string.h>

#include "libhexabus/sm/machine.hpp"

struct Label {
};

class Assembler {
	private:
		struct Fixup {
			uint16_t addr;
			const Label* label;
			bool is_short, is_absolute;
		};

		std::vector<uint8_t> _program;
		std::vector<Fixup> _fixups;

	public:
		Assembler& operator()(uint8_t val)
		{
			_program.push_back(val);
			return *this;
		}

		Assembler& operator()(enum hexabus::sm::hxb_sm_opcode op)
		{ return (*this)((uint8_t) op); }

		Assembler& operator()(uint16_t val)
		{ return (*this)((uint8_t) (val >> 8))((uint8_t) (val & 0xFF)); }

		Assembler& operator()(uint32_t val)
		{ return (*this)((uint16_t) (val >> 16))((uint16_t) (val & 0xFFFF)); }

		Assembler& operator()(float val)
		{
			uint32_t u;
			memcpy(&u, &val, 4);
			return (*this)(u);
		}

		Assembler& operator()(const Label& label, bool is_short, bool is_absolute = false)
		{
			Fixup f = { _program.size(), &label, is_short, is_absolute };
			_fixups.push_back(f);
			return is_short
				? (*this)((uint8_t) 0)
				: (*this)((uint16_t) 0);
		}

		Assembler& mark(const Label& label)
		{
			for (size_t i = 0; i < _fixups.size(); i++) {
				if (_fixups[i].label != &label)
					continue;

				uint16_t diff = _fixups[i].is_absolute
					? _program.size()
					: _program.size() - _fixups[i].addr;

				if (_fixups[i].is_short) {
					_program[_fixups[i].addr] = diff;
				} else {
					_program[_fixups[i].addr + 0] = diff >> 8;
					_program[_fixups[i].addr + 1] = diff & 0xFF;
				}
			}

			return *this;
		}

		std::vector<uint8_t> finish()
		{
			return _program;
		}
};

int main()
{
	using namespace hexabus::sm;

	Assembler as;
	Label on_packet, on_periodic, write_failed;

	as
		((uint8_t) 0)
		(on_packet, false, true)
		(on_periodic, false, true)
	.mark(on_periodic)
		(HSO_LD_U8)((uint8_t) 5)
		(HSO_LD_U32)((uint32_t) 2342)
		(HSO_LD_U8)((uint8_t) 5)
		(HSO_OP_MOD)
		(HSO_CONV_F)
		(HSO_LD_FLOAT)(3.1415926f)
		(HSO_OP_MUL)
		(HSO_WRITE)
		(HSO_JNZ_S)(write_failed, true)
		(HSO_LD_U8)((uint8_t) 5)
		(HSO_RET_CHANGE)
	.mark(on_packet)
	.mark(write_failed)
		(HSO_LD_U8)((uint8_t) 7)
		(HSO_RET_CHANGE);

	Machine machine(as.finish());

	machine.run_sm(0, 0, 0);

	return 0;
}
