#include "MC/disassembler.hpp"

#include "IR/builder.hpp"
#include "IR/program.hpp"
#include "Util/memorybuffer.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <map>

#include <boost/format.hpp>

namespace {

struct RawInstruction {
	size_t pos;

	hbt::ir::Opcode op;

	bool isValid, dtMaskInvalid, operandMissing, blockCmpMaskInvalid;
	bool isRelativeLabel, isSwitchTable;

	size_t nextPos;

	hbt::ir::Builder::immediate_t immed;
};

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

struct RawProgram {
	uint8_t version;
	std::array<uint8_t, 16> machineID;

	size_t onPacketPos, onPeriodicPos, onInitPos;

	std::vector<RawInstruction> instructions;
};

RawProgram parseRaw(const hbt::util::MemoryBuffer& program)
{
	using namespace hbt::ir;

	if (program.size() < 16 + 1 + 2 + 2)
		throw InvalidProgram("program too short", "");

	auto pit = program.crange<uint8_t>().begin();
	const auto pbegin = program.crange<uint8_t>().begin();
	const auto pend = program.crange<uint8_t>().end();

	auto read_u8 = [&] () -> uint8_t {
		auto r = *pit;
		++pit;
		return r;
	};

	auto read_u16 = [&] () -> uint16_t {
		uint8_t a = read_u8();
		uint8_t b = read_u8();
		return (a << 8) | b;
	};

	auto read_u32 = [&] () -> uint32_t {
		uint16_t a = read_u16();
		uint16_t b = read_u16();
		return (a << 16) | b;
	};

	auto read_f = [&] () {
		uint32_t u = read_u32();
		float f;
		memcpy(&f, &u, 4);
		return f;
	};

	auto readJumpOffset = [&] () {
		return Label(read_u16());
	};

	auto canRead = [&] (size_t count) {
		size_t pos = pit - pbegin;
		return pos + count <= program.size();
	};

	RawProgram result;

	std::copy(pit, pit + 16, result.machineID.begin());
	pit += 16;

	const auto programBegin = pit;

	result.version = read_u8();
	if (result.version != 0) {
		auto&& extra = (boost::format("got version %1%") % result.version).str();
		throw InvalidProgram("unknown program version", extra);
	}

	result.onInitPos = read_u16();
	result.onPacketPos = read_u16();
	result.onPeriodicPos = read_u16();

	for (;;) {
		size_t insnPos = size_t(pit - programBegin);

		if (result.instructions.size())
			result.instructions.back().nextPos = insnPos;

		if (pit == pend)
			break;

		result.instructions.push_back({ insnPos, Opcode(*pit) });
		++pit;

		RawInstruction& insn = result.instructions.back();

		switch (insn.op) {
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
			insn.isValid = true;
			break;

		case Opcode::LD_U8:
		case Opcode::DUP_I:
		case Opcode::ROT_I:
			if (!canRead(1))
				break;
			insn.immed = read_u8();
			insn.isValid = true;
			break;

		case Opcode::LD_MEM:
		case Opcode::ST_MEM: {
			if (!canRead(2))
				break;

			uint16_t ref = read_u16();
			MemType type = MemType(ref >> 12);
			uint16_t addr = ref & 0xfff;

			insn.immed = std::make_tuple(type, addr);
			insn.isValid = true;
			break;
		}

		case Opcode::LD_U16:
			if (!canRead(2))
				break;
			insn.immed = read_u16();
			insn.isValid = true;
			break;

		case Opcode::LD_U32:
			if (!canRead(4))
				break;
			insn.immed = read_u32();
			insn.isValid = true;
			break;

		case Opcode::LD_FLOAT:
			if (!canRead(4))
				break;
			insn.immed = read_f();
			insn.isValid = true;
			break;

		case Opcode::LD_DT: {
			if (!canRead(1))
				break;

			DTMask mask = DTMask(read_u8());
			unsigned second = 0;
			unsigned minute = 0;
			unsigned hour = 0;
			unsigned day = 0;
			unsigned month = 0;
			unsigned year = 0;
			unsigned weekday = 0;
			unsigned dtLength = 0;

			insn.dtMaskInvalid = invalid(mask);

			if (DTMask::second == (mask & DTMask::second)) dtLength++;
			if (DTMask::minute == (mask & DTMask::minute)) dtLength++;
			if (DTMask::hour == (mask & DTMask::hour)) dtLength++;
			if (DTMask::day == (mask & DTMask::day)) dtLength++;
			if (DTMask::month == (mask & DTMask::month)) dtLength++;
			if (DTMask::year == (mask & DTMask::year)) dtLength += 2;
			if (DTMask::weekday == (mask & DTMask::weekday)) dtLength++;

			if (!canRead(dtLength))
				break;

			if (DTMask::second == (mask & DTMask::second)) second = read_u8();
			if (DTMask::minute == (mask & DTMask::minute)) minute = read_u8();
			if (DTMask::hour == (mask & DTMask::hour)) hour = read_u8();
			if (DTMask::day == (mask & DTMask::day)) day = read_u8();
			if (DTMask::month == (mask & DTMask::month)) month = read_u8();
			if (DTMask::year == (mask & DTMask::year)) year = read_u16();
			if (DTMask::weekday == (mask & DTMask::weekday)) weekday = read_u8();

			insn.immed = std::make_tuple(mask, DateTime(second, minute, hour, day, month, year, weekday));
			insn.isValid = true;
			break;
		}

		case Opcode::DT_DECOMPOSE: {
			if (!canRead(1))
				break;

			DTMask mask = DTMask(read_u8());

			insn.dtMaskInvalid = invalid(mask);
			insn.isValid = !insn.dtMaskInvalid;
			insn.immed = mask;
			break;
		}

		case Opcode::SWITCH_8:
		case Opcode::SWITCH_16:
		case Opcode::SWITCH_32: {
			if (!canRead(1))
				break;

			uint8_t labels = read_u8();

			auto readLabel = [&] () -> unsigned {
				switch (insn.op) {
				case Opcode::SWITCH_8: return read_u8();
				case Opcode::SWITCH_16: return read_u16();
				case Opcode::SWITCH_32: return read_u32();
				default: throw std::runtime_error("opcode missing");
				}
			};

			unsigned itemSize = 0;
			switch (insn.op) {
			case Opcode::SWITCH_8: itemSize = 1 + 2; break;
			case Opcode::SWITCH_16: itemSize = 2 + 2; break;
			case Opcode::SWITCH_32: itemSize = 4 + 2; break;
			default: throw std::runtime_error("opcode missing");
			}

			if (!canRead(itemSize * labels))
				break;

			std::vector<SwitchEntry> entries;

			entries.reserve(labels);
			while (labels--) {
				entries.push_back({ readLabel(), readJumpOffset() });
			}

			insn.immed = SwitchTable(entries.begin(), entries.end());
			insn.isValid = true;
			insn.isSwitchTable = true;
			break;
		}

		case Opcode::CMP_BLOCK: {
			if (!canRead(1))
				break;

			uint8_t desc = read_u8();

			unsigned start = desc >> 4;
			unsigned end = desc & 0xF;
			unsigned length = end - start + 1;

			if (start > end) {
				insn.blockCmpMaskInvalid = true;
				break;
			}

			if (!canRead(length))
				break;

			std::array<uint8_t, 16> block;

			std::copy(pit, pit + length, block.begin());
			pit += length;

			insn.isValid = true;
			insn.immed = BlockPart(start, length, block);
			break;
		}

		case Opcode::JNZ:
		case Opcode::JZ:
		case Opcode::JUMP:
			if (!canRead(2))
				break;

			insn.immed = readJumpOffset();
			insn.isValid = true;
			insn.isRelativeLabel = true;
			break;
		}

		if (!insn.isValid && pit == pend) {
			insn.operandMissing = true;
			break;
		}
	}

	return result;
}

std::map<size_t, hbt::ir::Label> resolveLabels(const RawProgram& program, hbt::ir::Builder& builder)
{
	using namespace hbt::ir;

	std::map<size_t, Label> result;

	for (auto& insn : program.instructions) {
		if ((insn.pos == program.onPacketPos || insn.pos == program.onPeriodicPos || insn.pos == program.onInitPos) &&
				!result.count(insn.pos))
			result.insert({ insn.pos, builder.createLabel() });

		if (insn.isRelativeLabel) {
			Label l = boost::get<Label>(insn.immed);
			if (!result.count(insn.nextPos + l.id()))
				result.insert({ insn.nextPos + l.id(), builder.createLabel() });
		}
		if (insn.isSwitchTable) {
			const SwitchTable& st = boost::get<SwitchTable>(insn.immed);
			for (const auto& e : st) {
				if (!result.count(insn.nextPos + e.target.id()))
					result.insert({ insn.nextPos + e.target.id(), builder.createLabel() });
			}
		}
	}

	return result;
}

}

namespace hbt {
namespace mc {

std::unique_ptr<ir::Program> disassemble(const hbt::util::MemoryBuffer& program, bool ignoreInvalid)
{
	using namespace hbt::ir;

	RawProgram raw = parseRaw(program);

	Builder builder(raw.version, raw.machineID);

	std::map<size_t, Label> labelAbsPositions = resolveLabels(raw, builder);

	if (raw.onInitPos != 0xffff) {
		if (!labelAbsPositions.count(raw.onInitPos))
			throw InvalidProgram("invalid on_init vector", "");
		builder.onInit(labelAbsPositions.at(raw.onInitPos));
	}

	if (!labelAbsPositions.count(raw.onPacketPos))
		throw InvalidProgram("invalid on_packet vector", "");
	builder.onPacket(labelAbsPositions.at(raw.onPacketPos));

	if (!labelAbsPositions.count(raw.onPeriodicPos))
		throw InvalidProgram("invalid on_periodic vector", "");
	builder.onPeriodic(labelAbsPositions.at(raw.onPeriodicPos));

	for (const auto& insn : raw.instructions) {
		if (!insn.isValid && !ignoreInvalid)
			throw InvalidProgram("invalid instruction", (boost::format("at %1%") % insn.pos).str());

		if (!insn.isValid) {
			std::string comment;

			auto append = [&comment] (const std::string& s) {
				if (comment.size())
					comment += ", ";
				comment += s;
			};

			if (insn.dtMaskInvalid)
				append("DTMask invalid");
			if (insn.operandMissing)
				append("operand data missing");
			if (insn.blockCmpMaskInvalid)
				append("block compare mask invalid");

			builder.appendInvalid(insn.op, comment, 0);

			continue;
		}

		boost::optional<Label> thisLabel;

		if (labelAbsPositions.count(insn.pos))
			thisLabel = labelAbsPositions.at(insn.pos);

		switch (insn.op) {
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
		case Opcode::CMP_IP_LO:
			builder.append(thisLabel, insn.op, 0);
			break;

		case Opcode::LD_U8:
		case Opcode::DUP_I:
		case Opcode::ROT_I:
			builder.append(thisLabel, insn.op, boost::get<uint8_t>(insn.immed), 0);
			break;

		case Opcode::LD_MEM:
		case Opcode::ST_MEM:
			builder.append(thisLabel, insn.op, boost::get<std::tuple<MemType, uint16_t>>(insn.immed), 0);
			break;

		case Opcode::LD_U16:
			builder.append(thisLabel, insn.op, boost::get<uint16_t>(insn.immed), 0);
			break;

		case Opcode::LD_U32:
			builder.append(thisLabel, insn.op, boost::get<uint32_t>(insn.immed), 0);
			break;

		case Opcode::LD_FLOAT:
			builder.append(thisLabel, insn.op, boost::get<float>(insn.immed), 0);
			break;

		case Opcode::LD_DT:
			builder.append(thisLabel, insn.op, boost::get<std::tuple<DTMask, DateTime>>(insn.immed), 0);
			break;

		case Opcode::DT_DECOMPOSE:
			builder.append(thisLabel, insn.op, boost::get<DTMask>(insn.immed), 0);
			break;

		case Opcode::CMP_BLOCK:
			builder.append(thisLabel, insn.op, boost::get<BlockPart>(insn.immed), 0);
			break;

		case Opcode::SWITCH_8:
		case Opcode::SWITCH_16:
		case Opcode::SWITCH_32: {
			std::vector<SwitchEntry> entries;

			const SwitchTable& immed = boost::get<SwitchTable>(insn.immed);

			entries.reserve(immed.size());
			for (auto& e : immed) {
				if (!labelAbsPositions.count(e.target.id() + insn.nextPos)) {
					if (!ignoreInvalid)
						throw InvalidProgram("invalid instruction", (boost::format("at %1%") % insn.pos).str());

					builder.appendInvalid(
						insn.op,
						(boost::format("invalid entry (%1%, %2%)") % e.label % e.target.id()).str(), 0);

					continue;
				}

				entries.push_back({ e.label, labelAbsPositions.at(e.target.id() + insn.nextPos) });
			}

			builder.append(thisLabel, insn.op, SwitchTable(entries.begin(), entries.end()), 0);
			break;
		}

		case Opcode::JNZ:
		case Opcode::JZ:
		case Opcode::JUMP: {
			size_t addr = boost::get<Label>(insn.immed).id() + insn.nextPos;

			if (!labelAbsPositions.count(addr)) {
				if (!ignoreInvalid)
					throw InvalidProgram("invalid instruction", (boost::format("at %1%") % insn.pos).str());

				builder.appendInvalid(
					insn.op,
					(boost::format("invalid jump offset %1%") % (addr - insn.nextPos)).str(), 0);

				continue;
			}

			builder.append(thisLabel, insn.op, labelAbsPositions.at(addr), 0);
			break;
		}

		default:
			throw std::runtime_error("opcode missing");
		}
	}

	return builder.finish();
}

}
}
