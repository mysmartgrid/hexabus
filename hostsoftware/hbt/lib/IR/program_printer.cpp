#include "IR/program_printer.hpp"

#include "IR/program.hpp"

#include <boost/format.hpp>
#include <sstream>

namespace {

template<typename It>
class HexBlockPrinter {
	private:
		It it, end;

	public:
		HexBlockPrinter(It it, It end)
			: it(it), end(end)
		{}

		template<typename Char, typename Traits>
		friend std::basic_ostream<Char, Traits>&
			operator<<(std::basic_ostream<Char, Traits>& out, const HexBlockPrinter& hex)
		{
			out << "0x";
			for (It it = hex.it; it != hex.end; ++it)
				out << boost::format("%02.2x") % unsigned(*it);
			return out;
		}
};

template<typename It>
HexBlockPrinter<It> hexBlock(It it, It end)
{
	return { it, end };
}

template<typename Range>
HexBlockPrinter<typename Range::const_iterator> hexBlock(const Range& r)
{
	return { r.begin(), r.end() };
}

template<typename Char, typename Traits>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, hbt::ir::Label l)
{
	return out << "L" << l.id();
}

template<typename Char, typename Traits>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, hbt::ir::DTMask m)
{
	using hbt::ir::DTMask;

	if (DTMask::second == (m & DTMask::second)) out << "s";
	if (DTMask::minute == (m & DTMask::minute)) out << "m";
	if (DTMask::hour == (m & DTMask::hour)) out << "h";
	if (DTMask::day == (m & DTMask::day)) out << "D";
	if (DTMask::month == (m & DTMask::month)) out << "M";
	if (DTMask::year == (m & DTMask::year)) out << "Y";
	if (DTMask::weekday == (m & DTMask::weekday)) out << "W";

	return out;
}

template<typename Char, typename Traits>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, hbt::ir::Opcode op)
{
	using hbt::ir::Opcode;

	switch (op) {
	case Opcode::MUL: out << "mul"; break;
	case Opcode::DIV: out << "div"; break;
	case Opcode::MOD: out << "mod"; break;
	case Opcode::ADD: out << "add"; break;
	case Opcode::SUB: out << "sub"; break;
	case Opcode::AND: out << "and"; break;
	case Opcode::OR: out << "or"; break;
	case Opcode::XOR: out << "xor"; break;
	case Opcode::NOT: out << "not"; break;
	case Opcode::SHL: out << "shl"; break;
	case Opcode::SHR: out << "shr"; break;
	case Opcode::GETTYPE: out << "gettype"; break;
	case Opcode::DT_DIFF: out << "dt.diff"; break;
	case Opcode::CMP_IP_LO: out << "cmp.localhost"; break;
	case Opcode::CMP_LT: out << "cmp.lt"; break;
	case Opcode::CMP_LE: out << "cmp.le"; break;
	case Opcode::CMP_GT: out << "cmp.gt"; break;
	case Opcode::CMP_GE: out << "cmp.ge"; break;
	case Opcode::CMP_EQ: out << "cmp.eq"; break;
	case Opcode::CMP_NEQ: out << "cmp.neq"; break;
	case Opcode::CONV_B: out << "conv.b"; break;
	case Opcode::CONV_U8: out << "conv.u8"; break;
	case Opcode::CONV_U32: out << "conv.u32"; break;
	case Opcode::CONV_F: out << "conv.f"; break;
	case Opcode::WRITE: out << "write"; break;
	case Opcode::POP: out << "pop"; break;
	case Opcode::RET_CHANGE: out << "ret.change"; break;
	case Opcode::RET_STAY: out << "ret.stay"; break;
	case Opcode::LD_SOURCE_IP:
	case Opcode::LD_SOURCE_EID:
	case Opcode::LD_SOURCE_VAL:
	case Opcode::LD_CURSTATE:
	case Opcode::LD_CURSTATETIME:
	case Opcode::LD_FALSE:
	case Opcode::LD_TRUE:
	case Opcode::LD_SYSTIME:
	case Opcode::LD_U8:
	case Opcode::LD_U16:
	case Opcode::LD_U32:
	case Opcode::LD_FLOAT:
	case Opcode::LD_DT:
	case Opcode::LD_REG:
		out << "ld";
		break;
	case Opcode::ST_REG: out << "st"; break;
	case Opcode::DUP:
	case Opcode::DUP_I:
		out << "dup";
		break;
	case Opcode::ROT:
	case Opcode::ROT_I:
		out << "rot";
		break;
	case Opcode::SWITCH_8:
	case Opcode::SWITCH_16:
	case Opcode::SWITCH_32:
		out << "switch";
		break;
	case Opcode::DT_DECOMPOSE: out << "dt.decomp"; break;
	case Opcode::CMP_DT_LT: out << "cmp.dt.lt"; break;
	case Opcode::CMP_DT_GE: out << "cmp.dt.ge"; break;
	case Opcode::CMP_BLOCK: out << "cmp.block"; break;
	case Opcode::JNZ: out << "jnz"; break;
	case Opcode::JZ: out << "jz"; break;
	case Opcode::JUMP: out << "jump"; break;
	default:
		throw std::runtime_error("invalid program printed");
	}
	return out;
}

}

namespace hbt {
namespace ir {

std::string prettyPrint(const Program& program)
{
	using boost::format;

	std::stringstream out;

	out
		<< format(".version %1%\n") % unsigned(program.version())
		<< ".machine " << hexBlock(program.machine_id()) << "\n"
		<< ".on_packet " << program.onPacket() << "\n"
		<< ".on_periodic " << program.onPeriodic() << "\n";

	for (auto* insn : program.instructions()) {
		out << "\n";

		if (insn->label())
			out << *insn->label() << ":\n";
		out << "\t";

		out << insn->opcode();

		switch (insn->opcode()) {
		case Opcode::LD_SOURCE_IP: out << " src.ip"; break;
		case Opcode::LD_SOURCE_EID: out << " src.eid"; break;
		case Opcode::LD_SOURCE_VAL: out << " src.val"; break;
		case Opcode::LD_CURSTATE: out << " sys.state"; break;
		case Opcode::LD_CURSTATETIME: out << " sys.statetime"; break;
		case Opcode::LD_FALSE: out << " false"; break;
		case Opcode::LD_TRUE: out << " true"; break;
		case Opcode::LD_SYSTIME: out << " sys.time"; break;

		case Opcode::LD_U8:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint8_t>*>(insn)) {
				out << format(" u8(%1%)") % unsigned(i->immed());
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_U16:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint16_t>*>(insn)) {
				out << format(" u16(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_U32:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint32_t>*>(insn)) {
				out << format(" u32(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_FLOAT:
			if (auto* i = dynamic_cast<const ImmediateInstruction<float>*>(insn)) {
				out << format(" f(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_DT:
			if (auto* i = dynamic_cast<const ImmediateInstruction<std::tuple<DTMask, DateTime>>*>(insn)) {
				out << " dt(";

				bool skip = false;
				auto append = [&skip, &out] (char name, unsigned val) {
					if (skip)
						out << " ";
					out << format("%1%(%2%)") % name % val;
					skip = true;
				};
				auto has = [] (DTMask a, DTMask b) {
					return (a & b) == b;
				};

				DTMask mask = std::get<0>(i->immed());
				DateTime dt = std::get<1>(i->immed());

				if (has(mask, DTMask::second)) append('s', dt.second());
				if (has(mask, DTMask::minute)) append('m', dt.minute());
				if (has(mask, DTMask::hour)) append('h', dt.hour());
				if (has(mask, DTMask::day)) append('D', dt.day());
				if (has(mask, DTMask::month)) append('M', dt.month());
				if (has(mask, DTMask::year)) append('Y', dt.year());
				if (has(mask, DTMask::weekday)) append('W', dt.weekday());

				out << ")";
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_REG:
		case Opcode::ST_REG:
		case Opcode::DUP_I:
		case Opcode::ROT_I:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint8_t>*>(insn)) {
				format f;

				switch (insn->opcode()) {
				case Opcode::LD_REG: f = format(" [%1%]"); break;
				case Opcode::ST_REG: f = format(" [%1%]"); break;
				case Opcode::DUP_I: f = format(" %1%"); break;
				case Opcode::ROT_I: f = format(" %1%"); break;
				default: std::runtime_error("invalid program printed");
				}
				out << f % unsigned(i->immed());
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::SWITCH_8:
		case Opcode::SWITCH_16:
		case Opcode::SWITCH_32:
			if (auto* i = dynamic_cast<const ImmediateInstruction<SwitchTable>*>(insn)) {
				out << " {\n";
				for (const auto& entry : i->immed()) {
					out << "\t\t" << entry.label << ": " << entry.target << "\n";
				}
				out << "\t}";
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::DT_DECOMPOSE:
		case Opcode::CMP_DT_LT:
		case Opcode::CMP_DT_GE:
			if (auto* i = dynamic_cast<const ImmediateInstruction<DTMask>*>(insn)) {
				out << " " << i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::CMP_BLOCK:
			if (auto* i = dynamic_cast<const ImmediateInstruction<BlockPart>*>(insn)) {
				auto& block = i->immed().block();
				out << " (" << i->immed().start() << ", "
					<< hexBlock(block.begin(), block.begin() + i->immed().length())
					<< ")";
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::JNZ:
		case Opcode::JZ:
		case Opcode::JUMP:
			if (auto* i = dynamic_cast<const ImmediateInstruction<Label>*>(insn)) {
				out << " " << i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

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

	return out.str();
}

}
}
