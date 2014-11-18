#ifndef LIBHEXABUS_SM_MACHINE_HPP_
#define LIBHEXABUS_SM_MACHINE_HPP_

#include <vector>

#include <boost/signals2.hpp>

#include "libhexabus/sm/types.hpp"

namespace hexabus {
namespace sm {

class Machine {
	public:
		typedef boost::variant<
			bool,
			uint8_t, uint16_t, uint32_t, uint64_t,
			int8_t, int16_t, int32_t, int64_t,
			float> write_value_t;
		typedef boost::signals2::signal<uint8_t (uint32_t, write_value_t)> write_signal_t;

	private:
		enum {
			SM_STACK_SIZE = 32,
			SM_MEMORY_SIZE = 4096,
		};

		uint8_t sm_memory[SM_MEMORY_SIZE];
		hxb_sm_value_t sm_stack[SM_STACK_SIZE];
		uint32_t sm_first_run;

		std::vector<uint8_t> _program;
		uint64_t _created_at;
		write_signal_t _on_write;

		int sm_get_block(uint16_t at, uint8_t size, void* block);
		int sm_get_u8(uint16_t at, uint32_t* u);
		int sm_get_u16(uint16_t at, uint32_t* u);
		int sm_get_u32(uint16_t at, uint32_t* u);
		int sm_get_s8(uint16_t at, int32_t* s);
		int sm_get_s16(uint16_t at, int32_t* s);
		int sm_get_s32(uint16_t at, int32_t* s);
		uint8_t sm_endpoint_write(uint32_t eid, const hxb_sm_value_t* val);
		static int64_t sm_get_systime();
		void sm_diag_msg(int code, const char* file, int line);

		int sm_get_instruction(uint16_t at, struct hxb_sm_instruction* op);
		int sm_load_mem(const struct hxb_sm_instruction* insn, hxb_sm_value_t* value);
		int sm_store_mem(const struct hxb_sm_instruction* insn, hxb_sm_value_t value);

	public:
		Machine(const std::vector<uint8_t>& program)
			: sm_first_run(true), _program(program), _created_at(sm_get_systime())
		{
		}

		int run_sm(const char* src_ip, uint32_t eid, const hxb_sm_value_t* val);

		boost::signals2::connection onWrite(write_signal_t::slot_type slot)
		{
			return _on_write.connect(slot);
		}
};

}
}

#endif
