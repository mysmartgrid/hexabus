#include "hbt/IR/moduleprinter.hpp"

#include <sstream>

#include "hbt/IR/module.hpp"

namespace hbt {
namespace ir {

static void printInstruction(const Instruction* insn, std::ostream& out)
{
	auto typeName = [] (hbt::ir::Type t) {
		switch (t) {
		case hbt::ir::Type::Bool: return "bool";
		case hbt::ir::Type::UInt8: return "u8";
		case hbt::ir::Type::UInt16: return "u16";
		case hbt::ir::Type::UInt32: return "u32";
		case hbt::ir::Type::UInt64: return "u64";
		case hbt::ir::Type::Int8: return "s8";
		case hbt::ir::Type::Int16: return "s16";
		case hbt::ir::Type::Int32: return "s32";
		case hbt::ir::Type::Int64: return "s64";
		case hbt::ir::Type::Float:
		default:					return "float";
		}
	};

	switch (insn->insnType()) {
	case InsnType::Arithmetic: {
		auto* a = static_cast<const ArithmeticInsn*>(insn);
		out << a->name() << " = ";
		switch (a->op()) {
		case ArithOp::Mul: out << "mul "; break;
		case ArithOp::Div: out << "div "; break;
		case ArithOp::Mod: out << "mod "; break;
		case ArithOp::Add: out << "add "; break;
		case ArithOp::Sub: out << "sub "; break;
		case ArithOp::And: out << "and "; break;
		case ArithOp::Or: out << "or "; break;
		case ArithOp::Xor: out << "xor "; break;
		case ArithOp::Shl: out << "shl "; break;
		case ArithOp::Shr: out << "shr "; break;
		case ArithOp::Lt: out << "lt "; break;
		case ArithOp::Le: out << "le "; break;
		case ArithOp::Gt: out << "gt "; break;
		case ArithOp::Ge: out << "ge "; break;
		case ArithOp::Eq: out << "eq "; break;
		case ArithOp::Neq: out << "neq "; break;
		}

		out << a->left()->name() << " " << a->right()->name();
		break;
	}
	case InsnType::Cast: {
		auto* c = static_cast<const CastInsn*>(insn);
		out << c->name() << " = cast " << c->value()->name() << " to " << typeName(c->type());
		break;
	}
	case InsnType::SpecialValue: {
		auto* s = static_cast<const LoadSpecialInsn*>(insn);
		out << s->name() << " = ";
		switch (s->val()) {
		case SpecialVal::SourceEID: out << "src.eid"; break;
		case SpecialVal::SourceVal: out << "src.val " << typeName(s->type()); break;
		case SpecialVal::SysTime: out << "sys.time"; break;
		}
		break;
	}
	case InsnType::IntegralConstant: {
		auto* i = static_cast<const LoadIntInsn*>(insn);
		out << i->name() << " = " << typeName(i->type()) << " " << i->value();
		break;
	}
	case InsnType::FloatConstant: {
		auto* f = static_cast<const LoadFloatInsn*>(insn);
		out << f->name() << " = " << typeName(f->type()) << " " << f->value();
		break;
	}
	case InsnType::Load: {
		auto* l = static_cast<const LoadInsn*>(insn);
		out << l->name() << " = load " << typeName(l->type()) << " " << l->address();
		break;
	}
	case InsnType::Store: {
		auto* s = static_cast<const StoreInsn*>(insn);
		out << "store " << s->value()->name() << " " << s->address();
		break;
	}
	case InsnType::Switch: {
		auto* s = static_cast<const SwitchInsn*>(insn);
		out << "switch " << s->value()->name();
		for (auto& l : s->labels())
			out << " [" << l.first << ", " << l.second->name() << "]";
		if (s->defaultLabel())
			out << " [default, " << s->defaultLabel()->name() << "]";
		break;
	}
	case InsnType::Jump:
		out << "jump " << static_cast<const JumpInsn*>(insn)->target()->name();
		break;
	case InsnType::Return:
		out << "ret";
		break;
	case InsnType::CompareIP: {
		auto* c = static_cast<const CompareIPInsn*>(insn);
		out << c->name() << " = cmpip " << c->start() << " (";
		for (unsigned int i = c->start(); i < c->start() + c->length(); i++) {
			if (i != c->start())
				out << " ";
			out << unsigned(c->block()[i]);
		}
		out << ")";
		break;
	}
	case InsnType::Write: {
		auto* w = static_cast<const WriteInsn*>(insn);
		out << "write " << w->eid() << " " << w->value()->name();
		break;
	}
	case InsnType::ExtractDatePart: {
		auto* e = static_cast<const ExtractDatePartInsn*>(insn);
		out << e->name() << " = extract ";
		switch (e->part()) {
		case DatePart::Second: out << "second"; break;
		case DatePart::Minute: out << "minute"; break;
		case DatePart::Hour: out << "hour"; break;
		case DatePart::Day: out << "day"; break;
		case DatePart::Month: out << "month"; break;
		case DatePart::Year: out << "year"; break;
		case DatePart::Weekday: out << "weekday"; break;
		}
		out << " from " << e->value()->name();
		break;
	}
	case InsnType::Phi: {
		auto* p = static_cast<const PhiInsn*>(insn);
		out << p->name() << " = phi " << typeName(p->type());
		for (auto& src : p->sources())
			out << " [" << src.first->name() << ", " << src.second->name() << "]";
		break;
	}
	}
}

static void printBlock(const BasicBlock* block, std::ostream& out)
{
	out << block->name() << ":";
	for (auto* insn : block->instructions())
		printInstruction(insn, out << "\n\t");
}

std::string prettyPrint(const ModuleBase& module)
{
	std::stringstream out;

	out << "@version " << module.version();

	if (module.onInit())
		out << "\n@onInit " << module.onInit()->name();
	if (module.onPacket())
		out << "\n@onPacket " << module.onPacket()->name();
	if (module.onPeriodic())
		out << "\n@onPeriodic " << module.onPeriodic()->name();

	for (auto* b : module.blocks())
		printBlock(b, out << "\n\n");

	return out.str();
}

}
}
