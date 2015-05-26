#include "hbt/MC/assembler.hpp"

#include "hbt/MC/program.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <map>

namespace {

struct Fixup {
	size_t position;
	ssize_t offsetFrom;

	hbt::mc::Label target;
};

}

namespace hbt {
namespace mc {

hbt::util::MemoryBuffer assemble(const Program& program)
{
	hbt::util::MemoryBuffer result;

	std::map<size_t, size_t> labelPositions;
	std::vector<Fixup> fixups;

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

	auto append_label = [&fixups, &result, &append_u16] (mc::Label target) {
		fixups.push_back({ result.size(), -1, target });
		append_u16(0);
	};

	append_u8(program.version());

	if (program.onInit())
		fixups.push_back({ result.size(), 0, *program.onInit() });
	append_u16(0xffff);
	if (program.onPacket())
		fixups.push_back({ result.size(), 0, *program.onPacket() });
	append_u16(0xffff);
	if (program.onPeriodic())
		fixups.push_back({ result.size(), 0, *program.onPeriodic() });
	append_u16(0xffff);

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
		case Opcode::DUP_I:
		case Opcode::ROT_I:
		case Opcode::POP_I:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint8_t>*>(insn)) {
				append_u8(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_MEM:
		case Opcode::ST_MEM:
			if (auto* i = dynamic_cast<const ImmediateInstruction<std::tuple<MemType, uint16_t>>*>(insn)) {
				MemType type = std::get<0>(i->immed());
				uint16_t addr = std::get<1>(i->immed());
				append_u16((uint16_t(type) << 12) | (addr & 0xfff));
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

		case Opcode::LD_U64:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint64_t>*>(insn)) {
				append_u32(i->immed() >> 32);
				append_u32(i->immed() & 0xFFFFFFFF);
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_S8:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int8_t>*>(insn)) {
				append_u8(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_S16:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int16_t>*>(insn)) {
				append_u16(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_S32:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int32_t>*>(insn)) {
				append_u32(i->immed());
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_S64:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int64_t>*>(insn)) {
				append_u32(uint64_t(i->immed()) >> 32);
				append_u32(uint64_t(i->immed()) & 0xFFFFFFFF);
				break;
			}
			throw std::runtime_error("invalid program assembled");

		case Opcode::LD_FLOAT:
			if (auto* i = dynamic_cast<const ImmediateInstruction<float>*>(insn)) {
				append_f(i->immed());
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

		case Opcode::CMP_SRC_IP:
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
