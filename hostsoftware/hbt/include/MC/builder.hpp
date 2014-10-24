#ifndef INCLUDE_MC_BUILDER_HPP_3D3CABD64D9DF80F
#define INCLUDE_MC_BUILDER_HPP_3D3CABD64D9DF80F

#include "MC/instruction.hpp"

#include <list>
#include <map>
#include <memory>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace hbt {
namespace mc {

class Program;

class Builder {
	public:
		typedef boost::variant<
				uint8_t,
				uint16_t,
				uint32_t,
				uint64_t,
				int8_t,
				int16_t,
				int32_t,
				int64_t,
				float,
				SwitchTable,
				BlockPart,
				DTMask,
				Label,
				std::tuple<MemType, uint16_t>
			> immediate_t;

	private:
		size_t _labelMax;
		std::map<size_t, unsigned> _marked;
		std::list<std::unique_ptr<Instruction>> _instructions;

		uint8_t _version;
		std::array<uint8_t, 16> _machine_id;
		boost::optional<Label> _on_packet, _on_periodic, _on_init;

		void appendInstruction(boost::optional<Label> l, Opcode op, const immediate_t* immed,
				unsigned line);

	public:
		Builder(uint8_t version, const std::array<uint8_t, 16>& machine_id);

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

		Label createLabel(const std::string& name = "")
		{
			return Label(_labelMax++, name);
		}

		Label createLabel(const Label& old, const std::string& name = "")
		{
			return Label(old.id(), name);
		}

		void onPacket(const Label& l)
		{
			_on_packet = l;
		}

		void onPeriodic(const Label& l)
		{
			_on_periodic = l;
		}

		void onInit(const Label& l)
		{
			_on_init = l;
		}

		std::unique_ptr<Program> finish();
};

}
}

#endif
