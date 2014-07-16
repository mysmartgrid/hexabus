#include "IR/builder.hpp"
#include "IR/program.hpp"

#include <boost/format.hpp>

namespace hbt {
namespace ir {

Builder::Builder(uint8_t version, const std::array<uint8_t, 16>& machine_id)
	: _labelMax(0), _version(version), _machine_id(machine_id)
{
	if (version != 0)
		throw std::invalid_argument("version");
}

Builder::~Builder()
{
	for (Instruction* i : _instructions)
		delete i;
}

void Builder::appendInstruction(boost::optional<Label> l, Opcode op, const immediate_t* immed, unsigned line)
{
	using boost::get;

	if (l && _marked.count(l->id())) {
		auto&& extra = (
			boost::format("label %1% defined in lines %2% and %3%")
				% l->id() % _marked[l->id()] % line
			).str();
		throw InvalidProgram("label defined multpiple times", extra);
	}

	if (l)
		_marked.insert({ l->id(), line });

	switch (op) {
	case Opcode::LD_SOURCE_IP:
	case Opcode::LD_SOURCE_EID:
	case Opcode::LD_SOURCE_VAL:
	case Opcode::LD_CURSTATE:
	case Opcode::LD_CURSTATETIME:
	case Opcode::LD_FALSE:
	case Opcode::LD_TRUE:
	case Opcode::LD_SYSTIME:
	case Opcode::MUL:
	case Opcode::DIV:
	case Opcode::MOD:
	case Opcode::ADD:
	case Opcode::SUB:
	case Opcode::DT_DIFF:
	case Opcode::AND:
	case Opcode::OR:
	case Opcode::XOR:
	case Opcode::NOT:
	case Opcode::SHL:
	case Opcode::SHR:
	case Opcode::DUP:
	case Opcode::ROT:
	case Opcode::GETTYPE:
	case Opcode::CMP_IP_LO:
	case Opcode::CMP_LT:
	case Opcode::CMP_LE:
	case Opcode::CMP_GT:
	case Opcode::CMP_GE:
	case Opcode::CMP_EQ:
	case Opcode::CMP_NEQ:
	case Opcode::CONV_B:
	case Opcode::CONV_U8:
	case Opcode::CONV_U32:
	case Opcode::CONV_F:
	case Opcode::WRITE:
	case Opcode::POP:
	case Opcode::RET_CHANGE:
	case Opcode::RET_STAY:
		if (immed)
			throw std::invalid_argument("immed");

		_instructions.push_back(new Instruction(op, l, line));
		return;

	case Opcode::LD_U8:
	case Opcode::LD_REG:
	case Opcode::ST_REG:
	case Opcode::DUP_I:
	case Opcode::ROT_I:
		if (!get<uint8_t>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<uint8_t>(op, get<uint8_t>(*immed), l, line));
		return;

	case Opcode::LD_U16:
		if (!get<uint16_t>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<uint16_t>(op, get<uint16_t>(*immed), l, line));
		return;

	case Opcode::LD_U32:
		if (!get<uint32_t>(immed))
			throw std::invalid_argument("immed");

		if (get<uint32_t>(*immed) <= 65535) {
			_instructions.push_back(
				new ImmediateInstruction<uint16_t>(Opcode::LD_U16, get<uint32_t>(*immed), l, line));
		} else {
			_instructions.push_back(
				new ImmediateInstruction<uint32_t>(op, get<uint32_t>(*immed), l, line));
		}
		return;

	case Opcode::LD_FLOAT:
		if (!get<float>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<float>(op, get<float>(*immed), l, line));
		return;

	case Opcode::LD_DT:
		typedef std::tuple<DTMask, DateTime> ld_dt_immed;
		if (!get<ld_dt_immed>(immed) || invalid(std::get<0>(get<ld_dt_immed>(*immed))))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<ld_dt_immed>(op, get<ld_dt_immed>(*immed), l, line));
		return;

	case Opcode::DT_DECOMPOSE:
	case Opcode::CMP_DT_LT:
	case Opcode::CMP_DT_GE:
		if (!get<DTMask>(immed) || invalid(get<DTMask>(*immed)))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<DTMask>(op, get<DTMask>(*immed), l, line));
		return;

	case Opcode::SWITCH_8:
	case Opcode::SWITCH_16:
	case Opcode::SWITCH_32: {
		if (!get<SwitchTable>(immed))
			throw std::invalid_argument("immed");

		uint32_t maxLabel = 0;
		for (const auto& e : get<SwitchTable>(*immed))
			maxLabel = std::max(maxLabel, e.label);

		auto shortOp = maxLabel <= 255
			? Opcode::SWITCH_8
			: maxLabel <= 65535
				? Opcode::SWITCH_16
				: Opcode::SWITCH_32;

		_instructions.push_back(
			new ImmediateInstruction<SwitchTable>(shortOp, std::move(get<SwitchTable>(*immed)), l, line));
		return;
	}

	case Opcode::CMP_BLOCK:
		if (!get<BlockPart>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<BlockPart>(op, get<BlockPart>(*immed), l, line));
		return;

	case Opcode::JNZ:
	case Opcode::JZ:
	case Opcode::JUMP: {
		const Label* target = get<Label>(immed);
		if (!target)
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<Label>(op, *target, l, line));
		return;
	}
	}

	throw std::invalid_argument("op");
}

std::unique_ptr<Program> Builder::finish()
{
	std::map<size_t, unsigned> markedLabels;
	std::multimap<size_t, std::pair<Label, unsigned>> labelUses;

	auto useJumpLabel = [&] (Label to, const Instruction* insn) {
		if (markedLabels.count(to.id())) {
			auto&& extra = (
				boost::format("jump in line %1% to %2% in line %3%")
					% insn->line() % to.id() % markedLabels[to.id()]
				).str();
			throw InvalidProgram("backward jump not allowed", extra);
		}
		labelUses.insert({ to.id(), { to, insn->line() } });
	};

	for (const auto* insn : _instructions) {
		if (insn->label()) {
			markedLabels.insert({ insn->label()->id(), insn->line() });
			labelUses.erase(insn->label()->id());
		}

		if (auto* switchInsn = dynamic_cast<const ImmediateInstruction<SwitchTable>*>(insn)) {
			for (const SwitchEntry& e : switchInsn->immed())
				useJumpLabel(e.target, insn);

			continue;
		}

		if (auto* jumpInsn = dynamic_cast<const ImmediateInstruction<Label>*>(insn)) {
			useJumpLabel(jumpInsn->immed(), insn);

			continue;
		}
	}

	if (labelUses.size()) {
		std::string extra = "";
		for (const auto& use : labelUses) {
			if (extra.size())
				extra += ", ";
			extra += (boost::format("%1% (in line %2%)") % use.second.first.name() % use.second.second).str();
		}
		throw InvalidProgram("jump to undefined label", extra);
	}

	if (!_on_packet)
		throw InvalidProgram("vector table incomplete", "on_packet missing");
	if (!_on_periodic)
		throw InvalidProgram("vector table incomplete", "on_periodic missing");

	Program* p = new Program(
			_version, _machine_id, *_on_packet, *_on_periodic, _on_init,
			_instructions.begin(), _instructions.end());

	_instructions.clear();
	return std::unique_ptr<Program>(p);
}

}
}
