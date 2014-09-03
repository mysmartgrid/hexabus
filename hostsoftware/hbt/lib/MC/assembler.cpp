#include "MC/assembler.hpp"

#include "IR/program.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <map>

namespace {

struct Fixup {
	size_t position;
	ssize_t offsetFrom;

	hbt::ir::Label target;
};

}

namespace hbt {
namespace mc {

hbt::util::MemoryBuffer assemble(const ir::Program& program)
{
	using namespace hbt::ir;

	hbt::util::MemoryBuffer result;

	std::map<size_t, size_t> labelPositions;
	std::vector<Fixup> fixups;

	result.append(program.machine_id().begin(), program.machine_id().size());

	const size_t programBegin = result.size();

	auto append_u8 = [&result] (uint8_t u8) {
		result.append(&u8, 1);
	};

	auto append_u16 = [&append_u8] (uint16_t u16) {
		append_u8((u16 >> 8) & 0xFF);
		append_u8((u16 >> 0) & 0xFF);
	};

	auto append_u32 = [&append_u8] (uint32_t u32) {
		append_u8((u32 >> 24) & 0xFF);
		append_u8((u32 >> 16) & 0xFF);
		append_u8((u32 >>  8) & 0xFF);
		append_u8((u32 >>  0) & 0xFF);
	};

	auto append_f = [&append_u32] (float f) {
		uint32_t u;

		memcpy(&u, &f, 4);
		append_u32(u);
	};

	auto append_label = [&fixups, &result, &append_u16] (ir::Label target) {
		fixups.push_back({ result.size(), -1, target });
		append_u16(0);
	};

	append_u8(program.version());

	if (program.onInit())
		fixups.push_back({ result.size(), 0, *program.onInit() });
	append_u16(0xffff);
	fixups.push_back({ result.size(), 0, program.onPacket() });
	append_u16(0);
	fixups.push_back({ result.size(), 0, program.onPeriodic() });
	append_u16(0);

	for (auto* insn : program.instructions()) {
		if (insn->label())
			labelPositions.insert({ insn->label()->id(), result.size() - programBegin });

		for (auto it = fixups.rbegin(), end = fixups.rend(); it != end; ++it) {
			if (it->offsetFrom < 0)
				it->offsetFrom = result.size() - programBegin;
			else
				break;
		}

		append_u8(uint8_t(insn->opcode()));

		switch (insn->opcode()) {
		case Opcode::LD_U8:
		case Opcode::LD_REG:
		case Opcode::ST_REG:
		case Opcode::DUP_I:
		case Opcode::ROT_I:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint8_t>*>(insn)) {
				append_u8(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_U16:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint16_t>*>(insn)) {
				append_u16(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_U32:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint32_t>*>(insn)) {
				append_u32(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_FLOAT:
			if (auto* i = dynamic_cast<const ImmediateInstruction<float>*>(insn)) {
				append_f(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_DT:
			if (auto* i = dynamic_cast<const ImmediateInstruction<std::tuple<DTMask, DateTime>>*>(insn)) {
				auto has = [] (DTMask a, DTMask b) {
					return (a & b) == b;
				};

				DTMask mask = std::get<0>(i->immed());
				DateTime dt = std::get<1>(i->immed());

				append_u8(uint8_t(mask));

				if (has(mask, DTMask::second)) append_u8(dt.second());
				if (has(mask, DTMask::minute)) append_u8(dt.minute());
				if (has(mask, DTMask::hour)) append_u8(dt.hour());
				if (has(mask, DTMask::day)) append_u8(dt.day());
				if (has(mask, DTMask::month)) append_u8(dt.month());
				if (has(mask, DTMask::year)) append_u16(dt.year());
				if (has(mask, DTMask::weekday)) append_u8(dt.weekday());

				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::SWITCH_8:
		case Opcode::SWITCH_16:
		case Opcode::SWITCH_32:
			if (auto* i = dynamic_cast<const ImmediateInstruction<SwitchTable>*>(insn)) {
				append_u8(i->immed().size());

				for (const auto& entry : i->immed()) {
					switch (insn->opcode()) {
					case Opcode::SWITCH_8: append_u8(entry.label); break;
					case Opcode::SWITCH_16: append_u16(entry.label); break;
					case Opcode::SWITCH_32: append_u32(entry.label); break;
					default: throw std::runtime_error("opcode missing");
					}
					append_label(entry.target);
				}
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::DT_DECOMPOSE:
			if (auto* i = dynamic_cast<const ImmediateInstruction<DTMask>*>(insn)) {
				append_u8(uint8_t(i->immed()));
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::CMP_BLOCK:
			if (auto* i = dynamic_cast<const ImmediateInstruction<BlockPart>*>(insn)) {
				uint8_t start = i->immed().start();
				uint8_t end = start + i->immed().length() - 1;
				append_u8((start << 4) | end);

				result.append(i->immed().block().data(), i->immed().length());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::JNZ:
		case Opcode::JZ:
		case Opcode::JUMP:
			if (auto* i = dynamic_cast<const ImmediateInstruction<Label>*>(insn)) {
				append_label(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

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
		case Opcode::AND:
		case Opcode::OR:
		case Opcode::XOR:
		case Opcode::NOT:
		case Opcode::SHL:
		case Opcode::SHR:
		case Opcode::GETTYPE:
		case Opcode::DT_DIFF:
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
		case Opcode::DUP:
		case Opcode::ROT:
			break;
		}
	}

	for (const auto& f : fixups) {
		size_t pos = f.position;
		size_t to = labelPositions[f.target.id()];

		uint16_t fixupVal = htobe16(to - f.offsetFrom);

		result.replace(result.begin() + pos, &fixupVal, sizeof(fixupVal));
	}

	return result;
}

}
}
