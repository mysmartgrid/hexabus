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
	public:
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
			> immediate_t;

	private:
		std::set<size_t> _marked, _unmarked, _unmarked_used;
		std::list<Instruction*> _instructions;

		uint8_t _version;
		std::array<uint8_t, 16> _machine_id;
		boost::optional<Label> _on_packet, _on_periodic;

		void appendInstruction(boost::optional<Label> l, Opcode op, const immediate_t* immed);
		Label useLabel(Label l, bool forceForward = true);

	public:
		Builder(uint8_t version, const std::array<uint8_t, 16>& machine_id);

		Builder(const Builder&) = delete;
		Builder& operator=(const Builder&) = delete;

		Builder(Builder&& b)
		{
			*this = std::forward<Builder>(b);
		}

		Builder& operator=(Builder&&) = default;

		~Builder();

		void append(boost::optional<Label> l, Opcode op)
		{
			appendInstruction(l, op, nullptr);
		}

		void append(Opcode op)
		{
			appendInstruction(boost::none_t(), op, nullptr);
		}

		void append(boost::optional<Label> l, Opcode op, immediate_t&& immed)
		{
			appendInstruction(l, op, &immed);
		}

		void append(Opcode op, immediate_t&& immed)
		{
			appendInstruction(boost::none_t(), op, &immed);
		}

		void appendInvalid(Opcode op, const std::string& comment)
		{
			_instructions.push_back(new InvalidInstruction(op, comment));
		}

		Label createLabel()
		{
			Label l(_marked.size() + _unmarked.size());
			_unmarked.insert(l.id());
			return l;
		}

		void onPacket(Label l)
		{
			_on_packet = useLabel(l, false);
		}

		void onPeriodic(Label l)
		{
			_on_periodic = useLabel(l, false);
		}

		std::unique_ptr<Program> finish();
};

}
}

#endif
