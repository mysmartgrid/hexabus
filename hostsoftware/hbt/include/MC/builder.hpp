#ifndef INCLUDE_MC_BUILDER_HPP_3D3CABD64D9DF80F
#define INCLUDE_MC_BUILDER_HPP_3D3CABD64D9DF80F

#include "MC/instruction.hpp"

#include "Util/iterator.hpp"
#include "Util/range.hpp"

#include <map>
#include <memory>

#include <boost/container/list.hpp>
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
		typedef boost::container::list<std::unique_ptr<Instruction>> insn_list;
	public:
		typedef util::const_uptr_value_iterator<insn_list, Builder> insn_iterator;

	private:
		size_t _labelMax;
		insn_list _instructions;
		insn_iterator _insertPos;

		uint8_t _version;
		boost::optional<Label> _on_packet, _on_periodic, _on_init;

		insn_iterator insertInstruction(boost::optional<Label> l, Opcode op,
			const immediate_t* immed, unsigned line);

	public:
		Builder(uint8_t version);

		Builder(Program&& p);

		util::range<insn_iterator> instructions() const { return _instructions; }

		void erase(insn_iterator begin, insn_iterator end)
		{
			_instructions.erase(begin.baseIterator(), end.baseIterator());
		}

		insn_iterator moveRange(insn_iterator pos, insn_iterator begin, insn_iterator end)
		{
			_instructions.splice(
				pos.baseIterator(), _instructions,
				begin.baseIterator(), end.baseIterator());
			return begin;
		}

		void insertBefore(insn_iterator i) { _insertPos = i; }

		insn_iterator insert(boost::optional<Label> l, Opcode op, unsigned line)
		{
			return insertInstruction(l, op, nullptr, line);
		}

		insn_iterator insert(Opcode op, unsigned line)
		{
			return insertInstruction(boost::none_t(), op, nullptr, line);
		}

		insn_iterator insert(boost::optional<Label> l, Opcode op, immediate_t&& immed, unsigned line)
		{
			return insertInstruction(l, op, &immed, line);
		}

		insn_iterator insert(Opcode op, immediate_t&& immed, unsigned line)
		{
			return insertInstruction(boost::none_t(), op, &immed, line);
		}

		Label createLabel(const std::string& name = "")
		{
			return Label(_labelMax++, name);
		}

		void onPacket(const Label& l) { _on_packet = l; }
		void onPeriodic(const Label& l) { _on_periodic = l; }
		void onInit(const Label& l) { _on_init = l; }

		const Label* onPacket() const { return _on_packet.get_ptr(); }
		const Label* onPeriodic() const { return _on_periodic.get_ptr(); }
		const Label* onInit() const { return _on_init.get_ptr(); }

		std::unique_ptr<Program> finish();
};

}
}

#endif
