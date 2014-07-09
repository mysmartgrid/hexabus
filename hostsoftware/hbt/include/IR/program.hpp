#ifndef INCLUDE_IR_PROGRAM_HPP_345A0781A78039CE
#define INCLUDE_IR_PROGRAM_HPP_345A0781A78039CE

#include "IR/instruction.hpp"
#include "Util/range.hpp"

#include <vector>

namespace hbt {
namespace ir {

class InvalidProgram : public std::logic_error {
	using logic_error::logic_error;
};

class Program {
	private:
		uint8_t _version;
		std::array<uint8_t, 16> _machine_id;
		Label _on_packet, _on_periodic;
		std::vector<Instruction*> _instructions;

	public:
		typedef decltype(_instructions.cbegin()) iterator;

	public:
		template<typename It>
		Program(uint8_t version, const std::array<uint8_t, 16>& machine_id,
				Label on_packet, Label on_periodic, It begin, It end)
			: _version(version), _machine_id(machine_id), _on_packet(on_packet),
			  _on_periodic(on_periodic), _instructions(begin, end)
		{}

		~Program();

		uint8_t version() const { return _version; }
		const std::array<uint8_t, 16>& machine_id() const { return _machine_id; }
		Label onPacket() const { return _on_packet; }
		Label onPeriodic() const { return _on_periodic; }

		util::range<iterator> instructions() const { return util::make_range(_instructions); }
};

}
}

#endif
