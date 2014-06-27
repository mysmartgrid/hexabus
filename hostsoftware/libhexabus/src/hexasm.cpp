#include <utility>
#include <vector>

#include <string.h>

#include "libhexabus/sm/machine.hpp"
#include "libhexabus/socket.hpp"

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
					: _program.size() - _fixups[i].addr - 1;

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

static void on_packet(hexabus::sm::Machine& machine,
		const hexabus::Packet& packet,
		const boost::asio::ip::udp::endpoint& remote)
{
	using namespace hexabus;
	using namespace hexabus::sm;

	boost::asio::ip::address_v6::bytes_type addr;
	uint32_t eid;
	hxb_sm_value_t value;

	addr = remote.address().to_v6().to_bytes();
	if (const EIDPacket* p = dynamic_cast<const EIDPacket*>(&packet)) {
		eid = p->eid();
	} else {
		return;
	}

	if (const InfoPacket<bool>* p = dynamic_cast<const InfoPacket<bool>*>(&packet)) {
		value.type = p->datatype();
		value.v_uint = p->value();
	} else if (const InfoPacket<uint8_t>* p = dynamic_cast<const InfoPacket<uint8_t>*>(&packet)) {
		value.type = p->datatype();
		value.v_uint = p->value();
	} else if (const InfoPacket<uint32_t>* p = dynamic_cast<const InfoPacket<uint32_t>*>(&packet)) {
		value.type = p->datatype();
		value.v_uint = p->value();
	} else if (const InfoPacket<float>* p = dynamic_cast<const InfoPacket<float>*>(&packet)) {
		value.type = p->datatype();
		value.v_float = p->value();
	} else {
		std::cout << "Received non-sm packet" << std::endl;
		return;
	}

	std::cout << "Running state machine for packet: ";
	machine.run_sm((const char*) &addr[0], eid, &value);
}

static void on_periodic(hexabus::sm::Machine& machine, boost::asio::deadline_timer& timer)
{
	std::cout << "Running state machine for periodic check: ";
	machine.run_sm(NULL, 0, NULL);
	timer.async_wait(boost::bind(on_periodic, boost::ref(machine), boost::ref(timer)));
}

int main()
{
	using namespace hexabus::sm;

	typedef uint8_t u8;
	typedef uint16_t u16;
	typedef uint32_t u32;

	Assembler as;
	Label on_packet, on_periodic, my_packet, in_state_on, in_state_off, end_program;

	as
		((uint8_t) 0)
		(on_packet, false, true)
		(on_periodic, false, true)
	.mark(on_periodic)
		(HSO_RET_STAY)
	.mark(on_packet)
		(HSO_LD_SOURCE_IP)
		(HSO_CMP_BLOCK)((u8) 0x0f)((u32) 0xfdac814e)((u32) 0x24567334)((u32) 0x69ef7d2f)((u32) 0xcd3df75f)
		(HSO_LD_SOURCE_EID)
		(HSO_LD_U8)((u8) 20)
		(HSO_CMP_EQ)
		(HSO_OP_AND)
		(HSO_JNZ_S)(my_packet, true)
		(HSO_RET_STAY)
	.mark(my_packet)
		(HSO_LD_CURSTATE)
		(HSO_OP_SWITCH_8)((u8) 2)
//			((u8) 0)(in_state_off, false)
			((u8) 0)((u16) 1)
//			((u8) 1)(in_state_on, false)
			((u8) 1)((u16) (1 + 13))
		(HSO_RET_STAY)
	.mark(in_state_off)
		(HSO_LD_SOURCE_VAL)
		(HSO_LD_U8)((u8) 11)
		(HSO_CMP_EQ)
		(HSO_JZ_S)(end_program, true)
			(HSO_LD_U8)((u8) 2)
			(HSO_LD_TRUE)
			(HSO_WRITE)
			(HSO_POP)
		(HSO_LD_TRUE)
		(HSO_RET_CHANGE)
	.mark(in_state_on)
		(HSO_LD_SOURCE_VAL)
		(HSO_LD_U8)((u8) 12)
		(HSO_CMP_EQ)
		(HSO_JZ_S)(end_program, true)
			(HSO_LD_U8)((u8) 2)
			(HSO_LD_FALSE)
			(HSO_WRITE)
			(HSO_POP)
		(HSO_LD_FALSE)
		(HSO_RET_CHANGE)
	.mark(end_program)
		(HSO_RET_STAY)
	;

	Machine machine(as.finish());

	boost::asio::io_service io;
	hexabus::Listener listener(io);
	boost::asio::deadline_timer timer(io);

	listener.listen("eth1");
	listener.onPacketReceived(boost::bind(::on_packet, boost::ref(machine), _1, _2));

	::on_periodic(machine, timer);

	io.run();

	return 0;
}
