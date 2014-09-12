#ifndef LIBHEXABUS_SM_MACHINE_HPP_
#define LIBHEXABUS_SM_MACHINE_HPP_

#include <vector>

#include <boost/signals2.hpp>

#include "libhexabus/sm/types.hpp"

namespace hexabus {
namespace sm {

class Machine {
	public:
		typedef boost::variant<bool, uint8_t, uint32_t, float> write_value_t;
		typedef boost::signals2::signal<uint8_t (uint32_t, write_value_t)> write_signal_t;

	private:
		enum {
			SM_STACK_SIZE = 32,
			SM_MEMORY_SIZE = 4096,
		};

		uint8_t sm_memory[SM_MEMORY_SIZE];
		uint32_t sm_curstate, sm_in_state_since, sm_first_run;

		std::vector<uint8_t> _program;
		boost::posix_time::ptime _created_at;
		write_signal_t _on_write;

		int sm_get_block(uint16_t at, uint8_t size, void* block);
		int sm_get_u8(uint16_t at, uint32_t* u);
		int sm_get_u16(uint16_t at, uint32_t* u);
		int sm_get_u32(uint16_t at, uint32_t* u);
		int sm_get_float(uint16_t at, float* f);
		uint8_t sm_endpoint_write(uint32_t eid, const hxb_sm_value_t* val);
		uint32_t sm_get_timestamp();
		hxb_datetime_t sm_get_systime();
		void sm_diag_msg(int code, const char* file, int line);

		int sm_get_instruction(uint16_t at, struct hxb_sm_instruction* op);
		int sm_load_mem(const struct hxb_sm_instruction* insn, hxb_sm_value_t* value);
		int sm_store_mem(const struct hxb_sm_instruction* insn, const hxb_sm_value_t* value);

	public:
		Machine(const std::vector<uint8_t>& program)
			: sm_curstate(0), sm_in_state_since(0), sm_first_run(true),
			  _program(program),
			  _created_at(boost::posix_time::second_clock::local_time())
		{
		}

		int run_sm(const char* src_ip, uint32_t eid, const hxb_sm_value_t* val);

		uint32_t state() const { return sm_curstate; }

		boost::signals2::connection onWrite(write_signal_t::slot_type slot)
		{
			return _on_write.connect(slot);
		}
};

}
}

#endif
