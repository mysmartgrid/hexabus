#ifndef INCLUDE_IR_BUILDER_HPP_3D3CABD64D9DF80F
#define INCLUDE_IR_BUILDER_HPP_3D3CABD64D9DF80F

#include "IR/instruction.hpp"

#include <list>
#include <memory>
#include <set>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace hbt {
namespace ir {

class Program;

class InvalidJump : public std::logic_error {
	using logic_error::logic_error;
};

class Builder {
	private:
		std::set<size_t> _marked, _unmarked, _unmarked_used;
		std::list<Instruction*> _instructions;

		uint8_t _version;
		std::array<uint8_t, 16> _machine_id;
		boost::optional<Label> _on_packet, _on_periodic;

		typedef boost::variant<
				uint8_t,
				uint16_t,
				uint32_t,
				float,
				SwitchTable,
				BlockPart,
				DTMask,
				Label,
				std::tuple<DTMask, DateTime>
			> immed_t;

		void appendInstruction(Label* l, Opcode op, const immed_t* immed);
		Label useLabel(Label l);

	public:
		Builder(uint8_t version, const std::array<uint8_t, 16>& machine_id);

		~Builder();

		void append(Label l, Opcode op)
		{
			appendInstruction(&l, op, nullptr);
		}

		void append(Opcode op)
		{
			appendInstruction(nullptr, op, nullptr);
		}

		void append(Label l, Opcode op, immed_t&& immed)
		{
			appendInstruction(&l, op, &immed);
		}

		void append(Opcode op, immed_t&& immed)
		{
			appendInstruction(nullptr, op, &immed);
		}

		Label createLabel()
		{
			Label l(_marked.size() + _unmarked.size());
			_unmarked.insert(l.id());
			return l;
		}

		void onPacket(Label l)
		{
			_on_packet = useLabel(l);
		}

		void onPeriodic(Label l)
		{
			_on_periodic = useLabel(l);
		}

		std::unique_ptr<Program> finish();
};

}
}

#endif
