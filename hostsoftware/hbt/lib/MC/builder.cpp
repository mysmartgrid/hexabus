#include "MC/builder.hpp"
#include "MC/program.hpp"

#include <boost/format.hpp>

namespace hbt {
namespace mc {

Builder::Builder(uint8_t version, const std::array<uint8_t, 16>& machine_id)
	: _labelMax(0), _version(version), _machine_id(machine_id), _insertPos(_instructions.begin())
{
	if (version != 0)
		throw std::invalid_argument("version");
}

Builder::insn_iterator Builder::insertInstruction(boost::optional<Label> l, Opcode op,
	const immediate_t* immed, unsigned line)
{
	using boost::get;

	auto insertInsn = [&] (Instruction* i) -> insn_list::const_iterator {
		return _instructions.insert(_insertPos.baseIterator(), std::unique_ptr<Instruction>(i));
	};

	switch (op) {
	case Opcode::LD_SOURCE_IP:
	case Opcode::LD_SOURCE_EID:
	case Opcode::LD_SOURCE_VAL:
	case Opcode::LD_FALSE:
	case Opcode::LD_TRUE:
	case Opcode::LD_SYSTIME:
	case Opcode::MUL:
	case Opcode::DIV:
	case Opcode::MOD:
	case Opcode::ADD:
	case Opcode::SUB:
	case Opcode::AND:
	case Opcode::OR:
	case Opcode::XOR:
	case Opcode::SHL:
	case Opcode::SHR:
	case Opcode::DUP:
	case Opcode::ROT:
	case Opcode::CMP_IP_LO:
	case Opcode::CMP_LT:
	case Opcode::CMP_LE:
	case Opcode::CMP_GT:
	case Opcode::CMP_GE:
	case Opcode::CMP_EQ:
	case Opcode::CMP_NEQ:
	case Opcode::CONV_B:
	case Opcode::CONV_U8:
	case Opcode::CONV_U16:
	case Opcode::CONV_U32:
	case Opcode::CONV_U64:
	case Opcode::CONV_S8:
	case Opcode::CONV_S16:
	case Opcode::CONV_S32:
	case Opcode::CONV_S64:
	case Opcode::CONV_F:
	case Opcode::WRITE:
	case Opcode::POP:
	case Opcode::RET:
		if (immed)
			throw std::invalid_argument("immed");

		return insertInsn(new Instruction(op, l, line));

	case Opcode::LD_U8:
	case Opcode::DUP_I:
	case Opcode::ROT_I:
	case Opcode::EXCHANGE:
		if (auto* v = get<uint8_t>(immed))
			return insertInsn(new ImmediateInstruction<uint8_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_MEM:
	case Opcode::ST_MEM:
		if (auto* ref = get<std::tuple<MemType, uint16_t>>(immed)) {
			if (std::get<1>(*ref) > 0xfff)
				throw InvalidProgram("memory operand address invalid", "");

			return insertInsn(new ImmediateInstruction<std::tuple<MemType, uint16_t>>(op, *ref, l, line));
		}
		throw std::invalid_argument("immed");

	case Opcode::LD_U16:
		if (auto* v = get<uint16_t>(immed))
			return insertInsn(new ImmediateInstruction<uint16_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_U32:
		if (auto* v = get<uint32_t>(immed))
			return insertInsn(new ImmediateInstruction<uint32_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_U64:
		if (auto* v = get<uint64_t>(immed))
			return insertInsn(new ImmediateInstruction<uint64_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_S8:
		if (auto* v = get<int8_t>(immed))
			return insertInsn(new ImmediateInstruction<int8_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_S16:
		if (auto* v = get<int16_t>(immed))
			return insertInsn(new ImmediateInstruction<int16_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_S32:
		if (auto* v = get<int32_t>(immed))
			return insertInsn(new ImmediateInstruction<int32_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_S64:
		if (auto* v = get<int64_t>(immed))
			return insertInsn(new ImmediateInstruction<int64_t>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::LD_FLOAT:
		if (auto* v = get<float>(immed))
			return insertInsn(new ImmediateInstruction<float>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::DT_DECOMPOSE:
		if (!get<DTMask>(immed) || invalid(get<DTMask>(*immed)))
			throw std::invalid_argument("immed");

		return insertInsn(new ImmediateInstruction<DTMask>(op, get<DTMask>(*immed), l, line));

	case Opcode::SWITCH_8:
	case Opcode::SWITCH_16:
	case Opcode::SWITCH_32:
		if (auto* st = get<SwitchTable>(immed)) {
			uint32_t maxLabel = 0;
			for (const auto& e : *st)
				maxLabel = std::max(maxLabel, e.label);

			if ((op == Opcode::SWITCH_8 && maxLabel > 255)
					|| (op == Opcode::SWITCH_16 && maxLabel > 65535))
				throw std::invalid_argument("immed");

			return insertInsn(new ImmediateInstruction<SwitchTable>(op, std::move(*st), l, line));
		}
		throw std::invalid_argument("immed");

	case Opcode::CMP_BLOCK:
		if (auto* v = get<BlockPart>(immed))
			return insertInsn(new ImmediateInstruction<BlockPart>(op, *v, l, line));
		throw std::invalid_argument("immed");

	case Opcode::JNZ:
	case Opcode::JZ:
	case Opcode::JUMP:
		if (auto* target = get<Label>(immed))
			return insertInsn(new ImmediateInstruction<Label>(op, *target, l, line));
		throw std::invalid_argument("immed");
	}

	throw std::invalid_argument("op");
}

std::unique_ptr<Program> Builder::finish()
{
	std::map<size_t, unsigned> markedLabels;
	std::multimap<size_t, std::pair<Label, unsigned>> labelUses;

	auto useJumpLabel = [&] (Label to, const Instruction& insn) {
		if (markedLabels.count(to.id())) {
			auto&& extra = (
				boost::format("jump in line %1% to '%2%' in line %3%")
					% insn.line() % to.name() % markedLabels[to.id()]
				).str();
			throw InvalidProgram("backward jump not allowed", extra);
		}
		labelUses.insert({ to.id(), { to, insn.line() } });
	};

	for (const auto& insn : _instructions) {
		if (insn->label()) {
			if (!markedLabels.insert({ insn->label()->id(), insn->line() }).second) {
				auto&& extra = (
					boost::format("label '%1%' defined in lines %2% and %3%")
						% insn->label()->name() % markedLabels[insn->label()->id()] % insn->line()
					).str();
				throw InvalidProgram("label defined multiple times", extra);
			}
			labelUses.erase(insn->label()->id());
		}

		if (auto* switchInsn = dynamic_cast<const ImmediateInstruction<SwitchTable>*>(insn.get())) {
			for (const SwitchEntry& e : switchInsn->immed())
				useJumpLabel(e.target, *insn);

			continue;
		}

		if (auto* jumpInsn = dynamic_cast<const ImmediateInstruction<Label>*>(insn.get())) {
			useJumpLabel(jumpInsn->immed(), *insn);

			continue;
		}
	}

	if (labelUses.size()) {
		std::string extra = "";
		for (const auto& use : labelUses) {
			if (extra.size())
				extra += ", ";
			extra += (boost::format("'%1%' (in line %2%)") % use.second.first.name() % use.second.second).str();
		}
		throw InvalidProgram("jump to undefined label", extra);
	}

	return std::unique_ptr<Program>(
		new Program(
			_version, _machine_id, _on_packet, _on_periodic, _on_init,
			std::make_move_iterator(_instructions.begin()),
			std::make_move_iterator(_instructions.end())));
}

}
}
