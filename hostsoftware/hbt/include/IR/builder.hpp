#ifndef INCLUDE_IR_BUILDER_HPP_3D3CABD64D9DF80F
#define INCLUDE_IR_BUILDER_HPP_3D3CABD64D9DF80F

#include "IR/instruction.hpp"

#include <list>
#include <map>
#include <memory>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace hbt {
namespace ir {

class Program;

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
		size_t _labelMax;
		std::map<size_t, unsigned> _marked;
		std::list<Instruction*> _instructions;

		uint8_t _version;
		std::array<uint8_t, 16> _machine_id;
		boost::optional<Label> _on_packet, _on_periodic;

		void appendInstruction(boost::optional<Label> l, Opcode op, const immediate_t* immed,
				unsigned line);

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

		void append(boost::optional<Label> l, Opcode op, unsigned line)
		{
			appendInstruction(l, op, nullptr, line);
		}

		void append(Opcode op, unsigned line)
		{
			appendInstruction(boost::none_t(), op, nullptr, line);
		}

		void append(boost::optional<Label> l, Opcode op, immediate_t&& immed, unsigned line)
		{
			appendInstruction(l, op, &immed, line);
		}

		void append(Opcode op, immediate_t&& immed, unsigned line)
		{
			appendInstruction(boost::none_t(), op, &immed, line);
		}

		void appendInvalid(Opcode op, const std::string& comment, unsigned line)
		{
			_instructions.push_back(new InvalidInstruction(op, comment, line));
		}

		Label createLabel(const std::string& name = "")
		{
			return Label(_labelMax++, name);
		}

		void onPacket(Label l)
		{
			_on_packet = l;
		}

		void onPeriodic(Label l)
		{
			_on_periodic = l;
		}

		std::unique_ptr<Program> finish();
};

}
}

#endif
