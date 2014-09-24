#include <iostream>

#include "Util/memorybuffer.hpp"
#include "Lang/parser.hpp"
#include "Lang/ast.hpp"

#include <boost/asio/ip/address_v6.hpp>

using namespace hbt::lang;

static std::ostream& operator<<(std::ostream& o, Type t)
{
	switch (t) {
	case Type::Bool: return o << "bool";
	case Type::UInt8: return o << "uint8";
	case Type::UInt32: return o << "uint32";
	case Type::UInt64: return o << "uint64";
	case Type::Float: return o << "float";
	case Type::Unknown: return o << "<??"">";
	}
}

static std::ostream& operator<<(std::ostream& o, const Expr& e)
{
	if (auto u = dynamic_cast<const UnaryExpr*>(&e)) {
		switch (u->op()) {
		case UnaryOperator::Plus: o << "+" << u->expr(); break;
		case UnaryOperator::Minus: o << "-" << u->expr(); break;
		case UnaryOperator::Not: o << "!" << u->expr(); break;
		case UnaryOperator::Negate: o << "~" << u->expr(); break;
		default: o << "unknown unary operator\n"; exit(1); break;
		}
	} else if (auto b = dynamic_cast<const BinaryExpr*>(&e)) {
		o << "(" << b->left() << ") ";
		switch (b->op()) {
		case BinaryOperator::Plus: o << "+"; break;
		case BinaryOperator::Minus: o << "-"; break;
		case BinaryOperator::Multiply: o << "*"; break;
		case BinaryOperator::Divide: o << "/"; break;
		case BinaryOperator::Modulo: o << "%"; break;
		case BinaryOperator::And: o << "&"; break;
		case BinaryOperator::Or: o << "|"; break;
		case BinaryOperator::Xor: o << "^"; break;
		case BinaryOperator::BoolAnd: o << "&&"; break;
		case BinaryOperator::BoolOr: o << "||"; break;
		case BinaryOperator::Equals: o << "=="; break;
		case BinaryOperator::NotEquals: o << "!="; break;
		case BinaryOperator::LessThan: o << "<"; break;
		case BinaryOperator::LessOrEqual: o << "<="; break;
		case BinaryOperator::GreaterThan: o << ">"; break;
		case BinaryOperator::GreaterOrEqual: o << ">="; break;
		default: o << "unknown binary operator\n"; exit(1); break;
		}
		o << " (" << b->right() << ")";
	} else if (auto t = dynamic_cast<const TypedLiteral<uint64_t>*>(&e)) {
		o << t->value();
	} else if (auto t = dynamic_cast<const TypedLiteral<uint32_t>*>(&e)) {
		o << t->value();
	} else if (auto t = dynamic_cast<const TypedLiteral<uint8_t>*>(&e)) {
		o << t->value();
	} else if (auto t = dynamic_cast<const TypedLiteral<bool>*>(&e)) {
		o << t->value();
	} else if (auto t = dynamic_cast<const TypedLiteral<float>*>(&e)) {
		o << t->value();
	} else if (auto i = dynamic_cast<const TimeoutExpr*>(&e)) {
		o << "timeout";
	} else if (auto i = dynamic_cast<const IdentifierExpr*>(&e)) {
		o << i->name();
	} else if (auto c = dynamic_cast<const ConditionalExpr*>(&e)) {
		o << "(" << c->condition() << ") ? (" << c->ifTrue() << ") : (" << c->ifFalse() << ")";
	} else if (auto s = dynamic_cast<const PacketEIDExpr*>(&e)) {
		o << "packet_eid";
	} else if (auto ep = dynamic_cast<const EndpointExpr*>(&e)) {
		o << ep->device().name() << "." << ep->endpoint().name();
	} else if (auto c = dynamic_cast<const CallExpr*>(&e)) {
		o << c->name() << "(";
		bool only = true;
		for (auto& arg : c->arguments()) {
			if (!only)
				o << ", ";
			only = false;
			o << *arg;
		}
		o << ")";
	} else {
		o << "unknown expr " << typeid(e).name() << "\n";
		exit(1);
	}
	return o;
}

static std::ostream& operator<<(std::ostream& o, const Stmt& s)
{
	if (auto a = dynamic_cast<const AssignStmt*>(&s)) {
		o << a->target().name() << " = " << a->value() << ";\n";
	} else if (auto d = dynamic_cast<const DeclarationStmt*>(&s)) {
		o << d->type() << " " << d->name().name();
		if (d->value())
			o << " = " << *d->value();
		o << ";\n";
	} else if (auto b = dynamic_cast<const BlockStmt*>(&s)) {
		o << "{\n";
		for (auto& e : b->statements())
			o << *e;
		o << "}\n";
	} else if (auto i = dynamic_cast<const IfStmt*>(&s)) {
		o << "if (" << i->selector() << ") " << i->ifTrue();
		if (i->ifFalse())
			o << " else " << *i->ifFalse();
	} else if (auto w = dynamic_cast<const WriteStmt*>(&s)) {
		o << w->device().name() << "." << w->endpoint().name() << " := " << w->value() << ";\n";
	} else if (auto g = dynamic_cast<const GotoStmt*>(&s)) {
		o << "goto " << g->state().name() << ";\n";
	} else if (auto t = dynamic_cast<const SwitchStmt*>(&s)) {
		o << "switch (" << t->expr() << ") {\n";
		for (auto& e : t->entries()) {
			for (auto& l : e.labels())
				if (l)
					o << "	case " << *l << ":\n";
				else
					o << "	default:\n";
			o << " " << e.statement();
		}
	} else {
		o << "unknown stmt " << typeid(s).name() << "\n";
		exit(1);
	}
	return o;
}

static std::ostream& operator<<(std::ostream& o, const OnBlock& b)
{
	if (auto p = dynamic_cast<const OnPacketBlock*>(&b)) {
		o << "on packet from " << p->source().name() << " " << p->block();
	} else if (auto e = dynamic_cast<const OnExprBlock*>(&b)) {
		o << "on (" << e->condition() << ") " << e->block();
	} else {
		o << "on ";
		switch (b.trigger()) {
		case OnBlockTrigger::Entry: o << "entry "; break;
		case OnBlockTrigger::Exit: o << "exit "; break;
		case OnBlockTrigger::Periodic: o << "periodic "; break;
		default: o << "unknown trigger\n"; exit(1); break;
		}
		o << b.block();
	}
	return o;
}

static std::ostream& operator<<(std::ostream& o, const State& s)
{
	o << "state " << s.name().name() << " {\n";
	for (auto& d : s.variables())
		o << d;
	for (auto& b : s.onBlocks())
		o << *b;
	for (auto& st : s.statements())
		o << *st;
	o << "}\n";
	return o;
}

static std::ostream& operator<<(std::ostream& o, const MachineDefinition& d)
{
	o << "machine " << d.name().name() << " : {\n";
	for (auto& l : d.variables())
		o << l;
	for (auto& s : d.states())
		o << s;
	o << "}\n";
	return o;
}

static std::ostream& operator<<(std::ostream& o, const ProgramPart& p)
{
	if (auto m = dynamic_cast<const MachineDefinition*>(&p)) {
		o << "machine " << m->name().name() << " : {\n";
		for (auto& l : m->variables())
			o << l;
		for (auto& s : m->states())
			o << s;
		o << "}\n";
	} else if (auto m = dynamic_cast<const MachineInstantiation*>(&p)) {
		o << "machine " << m->name().name() << " : " << m->instanceOf().name() << "(";
		int f = 0;
		for (auto& a : m->arguments()) {
			if (f++)
				o << ", ";
			o << *a;
		}
		o << ")\n";
	} else if (auto m = dynamic_cast<const MachineClass*>(&p)) {
		o << "class " << m->name().name() << "(";
		bool first = true;
		for (auto& p : m->parameters()) {
			if (!first)
				o << ", ";
			first = false;
			o << p.name();
		}
		o << ") {\n";
		for (auto& l : m->variables())
			o << l;
		for (auto& s : m->states())
			o << s;
		o << "}\n";
	} else if (auto e = dynamic_cast<const Endpoint*>(&p)) {
		o << "endpoint " << e->name().name() << "(" << e->eid() << ") : " << e->type() << " (";
		int f = 0;
#define SINGLE(a, str) \
		do { \
			if ((a) == (e->access() & (a))) { \
				if (f++) o << ", "; \
				o << str; \
			} \
		} while (0)
		SINGLE(EndpointAccess::Read, "read");
		SINGLE(EndpointAccess::Write, "write");
		SINGLE(EndpointAccess::NonLocalWrite, "global_write");
		SINGLE(EndpointAccess::Broadcast, "broadcast");
#undef SINGLE
		o << ")\n";
	} else if (auto i = dynamic_cast<const IncludeLine*>(&p)) {
		o << "include \"" << i->file() << "\";\n";
	} else if (auto d = dynamic_cast<const Device*>(&p)) {
		o << "device " << d->name().name() << "(";
		boost::asio::ip::address_v6::bytes_type bytes;
		std::copy(d->address().begin(), d->address().end(), bytes.begin());
		boost::asio::ip::address_v6 addr(bytes);
		o << addr << ") : ";
		int f = 0;
		for (auto& e : d->endpoints()) {
			if (f++)
				o << ", ";
			o << e.name();
		}
		o << ";\n";
	} else {
		o << "unknown program part " << typeid(p).name() << "\n";
		exit(1);
	}
	return o;
}

int main()
{
	const char* expr = R"#(
class main(a, b, __123_usd) {
	state init {
		float x3;
		uint32 i9;

		on (snafu.zu > 8.1) {
			float f = 17;

			if (f > 5 && timeout < 76) {
				a.b := hour(dev.x);
				a.c := dev.value;
			} else a.c := packet_eid;
			switch (x) {
				case 1+1: goto z;
				default: case 8+hour(d): goto y;
			}
		}

		on entry {}
		if (x3 > 0)
			goto z;

	}
	state dead {
		// test
		/*
		 * bar
		 */
	}
}

include "../hb	t.hbh";

machine tree : main(4, dev);
machine tree : main();

endpoint button(4) : bool (broadcast, read, read);

device dev(fd01::2) : button;
//)#";

	hbt::util::MemoryBuffer buf(expr);

	try {
		auto tu = hbt::lang::parse(buf);

		for (auto& part : tu->items())
			std::cout << *part;
	} catch (const hbt::lang::ParseError& e) {
		std::cout << "hbt-lang: error in input\n"
			<< "expected " << e.expected() << " at " << e.line() << ":" << e.column();
		if (e.detail().size())
			std::cout << " (" << e.detail() << ")";
		std::cout << ", got " << e.got() << "\n";
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "hbt-lang: error: " << e.what() << "\n";
		return 1;
	}
}
