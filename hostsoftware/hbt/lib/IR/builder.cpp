#include "IR/builder.hpp"
#include "IR/program.hpp"

namespace hbt {
namespace ir {

Builder::Builder(uint8_t version, const std::array<uint8_t, 16>& machine_id)
	: _version(version), _machine_id(machine_id)
{
	if (version != 0)
		throw std::invalid_argument("version");
}

Builder::~Builder()
{
	for (Instruction* i : _instructions)
		delete i;
}

Label Builder::useLabel(Label l, bool forceForward)
{
	if (forceForward && _marked.count(l.id()))
		throw InvalidJump("backward jumps are forbidden");

	return l;
}

void Builder::appendInstruction(boost::optional<Label> l, Opcode op, const immediate_t* immed)
{
	using boost::get;

	if (l && _marked.count(l->id()))
		throw std::invalid_argument("l");

	if (l) {
		_marked.insert(l->id());
		_unmarked.erase(l->id());
		_unmarked_used.erase(l->id());
	}

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

		_instructions.push_back(new Instruction(op, l));
		return;

	case Opcode::LD_U8:
	case Opcode::LD_REG:
	case Opcode::ST_REG:
	case Opcode::DUP_I:
	case Opcode::ROT_I:
		if (!get<uint8_t>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<uint8_t>(op, get<uint8_t>(*immed), l));
		return;

	case Opcode::LD_U16:
		if (!get<uint16_t>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<uint16_t>(op, get<uint16_t>(*immed), l));
		return;

	case Opcode::LD_U32:
		if (!get<uint32_t>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<uint32_t>(op, get<uint32_t>(*immed), l));
		return;

	case Opcode::LD_FLOAT:
		if (!get<float>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<float>(op, get<float>(*immed), l));
		return;

	case Opcode::LD_DT:
		typedef std::tuple<DTMask, DateTime> ld_dt_immed;
		if (!get<ld_dt_immed>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<ld_dt_immed>(op, get<ld_dt_immed>(*immed), l));
		return;

	case Opcode::DT_DECOMPOSE:
	case Opcode::CMP_DT_LT:
	case Opcode::CMP_DT_GE:
		if (!get<DTMask>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<DTMask>(op, get<DTMask>(*immed), l));
		return;

	case Opcode::SWITCH_8:
	case Opcode::SWITCH_16:
	case Opcode::SWITCH_32:
		if (!get<SwitchTable>(immed))
			throw std::invalid_argument("immed");

		for (const auto& e : get<SwitchTable>(*immed))
			useLabel(e.target);

		_instructions.push_back(
			new ImmediateInstruction<SwitchTable>(op, std::move(get<SwitchTable>(*immed)), l));
		return;

	case Opcode::CMP_BLOCK:
		if (!get<BlockPart>(immed))
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<BlockPart>(op, get<BlockPart>(*immed), l));
		return;

	case Opcode::JNZ:
	case Opcode::JZ:
	case Opcode::JUMP: {
		const Label* target = get<Label>(immed);
		if (!target)
			throw std::invalid_argument("immed");

		_instructions.push_back(
			new ImmediateInstruction<Label>(op, useLabel(*target), l));
		return;
	}
	}

	throw std::invalid_argument("op");
}

std::unique_ptr<Program> Builder::finish()
{
	if (_unmarked_used.size())
		throw InvalidProgram("some labels are used, but not defined");

	if (!_on_packet || !_on_periodic)
		throw InvalidProgram("vector table incomplete");

	Program* p = new Program(
			_version, _machine_id, *_on_packet, *_on_periodic,
			std::make_move_iterator(_instructions.begin()),
			std::make_move_iterator(_instructions.end()));

	_instructions.clear();
	return std::unique_ptr<Program>(p);
}

}
}
