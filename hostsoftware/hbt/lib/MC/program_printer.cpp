#include "MC/program_printer.hpp"

#include "MC/program.hpp"

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
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, hbt::mc::Label l)
{
	if (l.name().size())
		return out << l.name();
	else
		return out << "L" << l.id();
}

template<typename Char, typename Traits>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, hbt::mc::DTMask m)
{
	using hbt::mc::DTMask;

	if (DTMask::second == (m & DTMask::second)) out << "s";
	if (DTMask::minute == (m & DTMask::minute)) out << "m";
	if (DTMask::hour == (m & DTMask::hour)) out << "h";
	if (DTMask::day == (m & DTMask::day)) out << "D";
	if (DTMask::month == (m & DTMask::month)) out << "M";
	if (DTMask::year == (m & DTMask::year)) out << "Y";
	if (DTMask::weekday == (m & DTMask::weekday)) out << "W";

	return out;
}

const char* opcodeName(hbt::mc::Opcode op)
{
	using hbt::mc::Opcode;

	switch (op) {
	case Opcode::MUL: return "mul";
	case Opcode::DIV: return "div";
	case Opcode::MOD: return "mod";
	case Opcode::ADD: return "add";
	case Opcode::SUB: return "sub";
	case Opcode::AND: return "and";
	case Opcode::OR: return "or";
	case Opcode::XOR: return "xor";
	case Opcode::SHL: return "shl";
	case Opcode::SHR: return "shr";
	case Opcode::CMP_IP_LO: return "cmp.localhost";
	case Opcode::CMP_LT: return "cmp.lt";
	case Opcode::CMP_LE: return "cmp.le";
	case Opcode::CMP_GT: return "cmp.gt";
	case Opcode::CMP_GE: return "cmp.ge";
	case Opcode::CMP_EQ: return "cmp.eq";
	case Opcode::CMP_NEQ: return "cmp.neq";
	case Opcode::CONV_B: return "conv.b";
	case Opcode::CONV_U8: return "conv.u8";
	case Opcode::CONV_U16: return "conv.u16";
	case Opcode::CONV_U32: return "conv.u32";
	case Opcode::CONV_U64: return "conv.u64";
	case Opcode::CONV_S8: return "conv.s8";
	case Opcode::CONV_S16: return "conv.s16";
	case Opcode::CONV_S32: return "conv.s32";
	case Opcode::CONV_S64: return "conv.s64";
	case Opcode::CONV_F: return "conv.f";
	case Opcode::WRITE: return "write";
	case Opcode::POP: return "pop";
	case Opcode::EXCHANGE: return "exchange";
	case Opcode::RET: return "ret";
	case Opcode::LD_SOURCE_IP:
	case Opcode::LD_SOURCE_EID:
	case Opcode::LD_SOURCE_VAL:
	case Opcode::LD_FALSE:
	case Opcode::LD_TRUE:
	case Opcode::LD_SYSTIME:
	case Opcode::LD_U8:
	case Opcode::LD_U16:
	case Opcode::LD_U32:
	case Opcode::LD_U64:
	case Opcode::LD_S8:
	case Opcode::LD_S16:
	case Opcode::LD_S32:
	case Opcode::LD_S64:
	case Opcode::LD_FLOAT:
	case Opcode::LD_MEM:
		return "ld";
	case Opcode::ST_MEM: return "st";
	case Opcode::DUP:
	case Opcode::DUP_I:
		return "dup";
	case Opcode::ROT:
	case Opcode::ROT_I:
		return "rot";
	case Opcode::SWITCH_8: return "switch8";
	case Opcode::SWITCH_16: return "switch16";
	case Opcode::SWITCH_32: return "switch32";
	case Opcode::DT_DECOMPOSE: return "dt.decomp";
	case Opcode::CMP_BLOCK: return "cmp.block";
	case Opcode::JNZ: return "jnz";
	case Opcode::JZ: return "jz";
	case Opcode::JUMP: return "jump";
	default: return nullptr;
	}
}

template<typename Char, typename Traits>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, hbt::mc::Opcode op)
{
	const char* name = opcodeName(op);
	if (name)
		return out << name;

	throw std::runtime_error("invalid program printed");
}

}

namespace hbt {
namespace mc {

std::string prettyPrint(const Program& program)
{
	using boost::format;

	std::stringstream out;

	out
		<< format(".version %1%\n") % unsigned(program.version())
		<< ".machine " << hexBlock(program.machine_id()) << "\n";

	if (program.onInit())
		out << ".on_init " << *program.onInit() << "\n";
	if (program.onPacket())
		out << ".on_packet " << *program.onPacket() << "\n";
	if (program.onPeriodic())
		out << ".on_periodic " << *program.onPeriodic() << "\n";

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

		case Opcode::LD_U64:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint64_t>*>(insn)) {
				out << format(" u64(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_S8:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int8_t>*>(insn)) {
				out << format(" s8(%1%)") % int(i->immed());
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_S16:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int16_t>*>(insn)) {
				out << format(" s16(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_S32:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int32_t>*>(insn)) {
				out << format(" s32(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_S64:
			if (auto* i = dynamic_cast<const ImmediateInstruction<int64_t>*>(insn)) {
				out << format(" s64(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_FLOAT:
			if (auto* i = dynamic_cast<const ImmediateInstruction<float>*>(insn)) {
				out << format(" f(%1%)") % i->immed();
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::LD_MEM:
		case Opcode::ST_MEM:
			if (auto* i = dynamic_cast<const ImmediateInstruction<std::tuple<MemType, uint16_t>>*>(insn)) {
				format f;

				switch (std::get<0>(i->immed())) {
				case MemType::Bool: f = format(" b[%1%]"); break;
				case MemType::U8: f = format(" u8[%1%]"); break;
				case MemType::U16: f = format(" u16[%1%]"); break;
				case MemType::U32: f = format(" u32[%1%]"); break;
				case MemType::U64: f = format(" u64[%1%]"); break;
				case MemType::S8: f = format(" s8[%1%]"); break;
				case MemType::S16: f = format(" s16[%1%]"); break;
				case MemType::S32: f = format(" s32[%1%]"); break;
				case MemType::S64: f = format(" s64[%1%]"); break;
				case MemType::Float: f = format(" f[%1%]"); break;
				}
				out << f % std::get<1>(i->immed());
				break;
			}
			throw std::runtime_error("invalid program printed");

		case Opcode::DUP_I:
		case Opcode::ROT_I:
		case Opcode::EXCHANGE:
			if (auto* i = dynamic_cast<const ImmediateInstruction<uint8_t>*>(insn)) {
				out << format(" %1%") % unsigned(i->immed());
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
		case Opcode::SHL:
		case Opcode::SHR:
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
		case Opcode::DUP:
		case Opcode::ROT:
			break;
		}
	}

	return out.str();
}

}
}
