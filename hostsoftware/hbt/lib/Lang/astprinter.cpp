#include "hbt/Lang/astprinter.hpp"

#include "hbt/Util/fatal.hpp"

#include <sstream>

#include "boost/asio/ip/address_v6.hpp"

namespace hbt {
namespace lang {

void ASTPrinter::printType(Type t, Expr* source)
{
	if (source) {
		out << "typeof(";
		source->accept(*this);
		out << ")";
	} else {
		out << typeName(t);
	}
}

void ASTPrinter::indent()
{
	if (!_skipIndent)
		out << std::string(_indent, '\t');

	_skipIndent = false;
}

void ASTPrinter::visit(IdentifierExpr& i)
{
	out << i.name();
}

void ASTPrinter::visit(TypedLiteral<bool>& l)
{
	out << (l.value() ? "true" : "false");
}

void ASTPrinter::visit(TypedLiteral<uint8_t>& l) { out << unsigned(l.value()); }
void ASTPrinter::visit(TypedLiteral<uint16_t>& l) { out << l.value(); }
void ASTPrinter::visit(TypedLiteral<uint32_t>& l) { out << l.value(); }
void ASTPrinter::visit(TypedLiteral<uint64_t>& l) { out << l.value(); }
void ASTPrinter::visit(TypedLiteral<int8_t>& l) { out << int(l.value()); }
void ASTPrinter::visit(TypedLiteral<int16_t>& l) { out << l.value(); }
void ASTPrinter::visit(TypedLiteral<int32_t>& l) { out << l.value(); }
void ASTPrinter::visit(TypedLiteral<int64_t>& l) { out << l.value(); }

void ASTPrinter::visit(TypedLiteral<float>& l)
{
	switch (std::fpclassify(l.value())) {
	case FP_NAN:
		out << "nan";
		return;

	case FP_INFINITE:
		out << (l.value() < 0 ? "-inf" : "inf");
		return;
	}

	auto doesRoundtrip = [&l] (unsigned precision) {
		float f;
		std::stringstream buf;

		buf.precision(precision);
		buf << l.value();
		buf >> f;
		return l.value() == f;
	};

	unsigned precision = 6;
	while (!doesRoundtrip(precision)) {
		precision += 10;
	}
	while (doesRoundtrip(precision) && precision > 6) {
		precision--;
	}
	precision++;

	std::stringstream buf;

	buf.precision(precision);
	buf << l.value();
	out << buf.str();
}

void ASTPrinter::visit(CastExpr& c)
{
	printType(c.type(), c.typeSource());
	out << "(";
	c.expr().accept(*this);
	out << ")";
}

void ASTPrinter::visit(UnaryExpr& u)
{
	switch (u.op()) {
	case UnaryOperator::Plus:   out << "+"; break;
	case UnaryOperator::Minus:  out << "-"; break;
	case UnaryOperator::Not:    out << "!"; break;
	case UnaryOperator::Negate: out << "~"; break;
	default: throw "unknown operator";
	}
	u.expr().accept(*this);
}

void ASTPrinter::visit(BinaryExpr& b)
{
	auto precedence = [] (BinaryOperator op) {
		switch (op) {
		case BinaryOperator::Multiply:
		case BinaryOperator::Divide:
		case BinaryOperator::Modulo:
			return 10;

		case BinaryOperator::Plus:
		case BinaryOperator::Minus:
			return 9;

		case BinaryOperator::ShiftLeft:
		case BinaryOperator::ShiftRight:
			return 8;

		case BinaryOperator::LessThan:
		case BinaryOperator::LessOrEqual:
		case BinaryOperator::GreaterThan:
		case BinaryOperator::GreaterOrEqual:
			return 7;
			
		case BinaryOperator::Equals:
		case BinaryOperator::NotEquals:
			return 6;

		case BinaryOperator::And:
			return 5;

		case BinaryOperator::Xor:
			return 4;

		case BinaryOperator::Or:
			return 3;

		case BinaryOperator::BoolAnd:
			return 2;

		case BinaryOperator::BoolOr:
			return 1;
		}

		hbt_unreachable();
	};

	auto wantParens = [&b, precedence] (Expr& other) {
		if (dynamic_cast<ConditionalExpr*>(&other))
			return true;

		if (auto* e = dynamic_cast<BinaryExpr*>(&other))
			return precedence(e->op()) < precedence(b.op());

		return false;
	};

	if (wantParens(b.left())) {
		out << "(";
		b.left().accept(*this);
		out << ")";
	} else {
		b.left().accept(*this);
	}

	switch (b.op()) {
	case BinaryOperator::Plus:           out << " + "; break;
	case BinaryOperator::Minus:          out << " - "; break;
	case BinaryOperator::Multiply:       out << " * "; break;
	case BinaryOperator::Divide:         out << " / "; break;
	case BinaryOperator::Modulo:         out << " % "; break;
	case BinaryOperator::And:            out << " & "; break;
	case BinaryOperator::Or:             out << " | "; break;
	case BinaryOperator::Xor:            out << " ^ "; break;
	case BinaryOperator::BoolAnd:        out << " && "; break;
	case BinaryOperator::BoolOr:         out << " || "; break;
	case BinaryOperator::Equals:         out << " == "; break;
	case BinaryOperator::NotEquals:      out << " != "; break;
	case BinaryOperator::LessThan:       out << " < "; break;
	case BinaryOperator::LessOrEqual:    out << " <= "; break;
	case BinaryOperator::GreaterThan:    out << " > "; break;
	case BinaryOperator::GreaterOrEqual: out << " >= "; break;
	case BinaryOperator::ShiftLeft:      out << " << "; break;
	case BinaryOperator::ShiftRight:     out << " >> "; break;
	default: throw "unknown operator";
	}

	if (wantParens(b.right())) {
		out << "(";
		b.right().accept(*this);
		out << ")";
	} else {
		b.right().accept(*this);
	}
}

void ASTPrinter::visit(ConditionalExpr& c)
{
	c.condition().accept(*this);
	out << " ? ";
	c.ifTrue().accept(*this);
	out << " : ";
	c.ifFalse().accept(*this);
}

void ASTPrinter::visit(EndpointExpr& e)
{
	unsigned count = 0;
	for (auto& id : e.path()) {
		if (count++)
			out << '.';
		out << id.name();
	}
}

void ASTPrinter::visit(CallExpr& c)
{
	out << c.name().name() << "(";
	unsigned idx = 0;
	for (auto& arg : c.arguments()) {
		if (idx++)
			out << ", ";
		arg->accept(*this);
	}
	out << ")";
}

void ASTPrinter::visit(AssignStmt& a)
{
	indent();

	out << a.target().name() << " = ";
	a.value().accept(*this);
	out << ";";
}

void ASTPrinter::visit(WriteStmt& w)
{
	indent();

	w.target().accept(*this);
	out << " = ";
	w.value().accept(*this);
	out << ";";
}

void ASTPrinter::visit(IfStmt& i)
{
	indent();

	out << "if (";
	i.condition().accept(*this);
	out << ")";

	bool trueBlock = dynamic_cast<BlockStmt*>(&i.ifTrue());

	if (!trueBlock) {
		out << "\n";
		_indent++;
	} else {
		out << " ";
		_skipIndent = true;
	}
	i.ifTrue().accept(*this);
	if (!trueBlock)
		_indent--;

	if (i.ifFalse()) {
		bool falseBlock = dynamic_cast<BlockStmt*>(i.ifFalse());
		if (trueBlock)
			out << " else ";
		else
			out << "\nelse\n";

		if (!falseBlock)
			_indent++;
		else
			_skipIndent = true;
		i.ifFalse()->accept(*this);
		if (!falseBlock)
			_indent--;
	}
}

void ASTPrinter::visit(SwitchStmt& s)
{
	indent();

	out << "switch (";
	s.expr().accept(*this);
	out << ") {\n";

	unsigned count = 0;
	for (auto& entry : s.entries()) {
		if (count++)
			out << "\n\n";

		unsigned lcount = 0;
		for (auto& label : entry.labels()) {
			if (lcount++)
				out << "\n";

			indent();
			if (label.expr()) {
				out << "case ";
				label.expr()->accept(*this);
				out << ":";
			} else {
				out << "default:";
			}
		}

		if (dynamic_cast<BlockStmt*>(&entry.statement())) {
			out << " ";
			_skipIndent = true;
			entry.statement().accept(*this);
		} else {
			out << "\n";
			_indent++;
			entry.statement().accept(*this);
			_indent--;
		}
	}

	out << "\n";
	indent();
	out << "}";
}

void ASTPrinter::visit(BlockStmt& b)
{
	indent();

	out << "{";
	_indent++;
	for (auto& s : b.statements()) {
		out << "\n";
		s->accept(*this);
	}
	_indent--;
	out << "\n";
	indent();
	out << "}";
}

void ASTPrinter::visit(DeclarationStmt& d)
{
	indent();

	printType(d.type(), d.typeSource());
	out << " " << d.name().name() << " = ";
	d.value().accept(*this);
	out << ";";
}

void ASTPrinter::visit(GotoStmt& g)
{
	indent();

	out << "goto " << g.state().name() << ";";
}

void ASTPrinter::printOnBlockBody(OnBlock& o)
{
	_indent++;
	for (auto& s : o.block().statements()) {
		out << "\n";
		s->accept(*this);
	}
	_indent--;
	out << "\n";
	indent();
	out << "}";
}

void ASTPrinter::visit(OnSimpleBlock& o)
{
	indent();

	switch (o.trigger()) {
	case OnSimpleTrigger::Entry:    out << "on entry "; break;
	case OnSimpleTrigger::Exit:     out << "on exit "; break;
	case OnSimpleTrigger::Periodic: out << "on periodic "; break;
	case OnSimpleTrigger::Always:   out << "always "; break;
	default: throw "unknown trigger";
	}
	out << "{";

	printOnBlockBody(o);
}

void ASTPrinter::visit(OnExprBlock& o)
{
	indent();

	out << "on (";
	o.condition().accept(*this);
	out << ") {";
	printOnBlockBody(o);
}

void ASTPrinter::visit(OnUpdateBlock& o)
{
	indent();

	out << "on update from ";
	o.endpoint().accept(*this);
	out << " {";
	printOnBlockBody(o);
}

void ASTPrinter::visit(Endpoint& e)
{
	out
		<< "endpoint " << e.name().name() << "(" << e.eid() << ") : "
		<< typeName(e.type()) << "(";

	unsigned count = 0;
	auto appendIf = [this, e, &count] (EndpointAccess a, const char* s) {
		if ((e.access() & a) == a) {
			if (count++)
				out << ", ";
			out << s;
		}
	};

	appendIf(EndpointAccess::Read, "read");
	appendIf(EndpointAccess::Write, "write");
	appendIf(EndpointAccess::NonLocalWrite, "global_write");
	appendIf(EndpointAccess::Broadcast, "broadcast");

	out << ");";
}

void ASTPrinter::visit(Device& d)
{
	out << "device " << d.name().name() << "(";

	boost::asio::ip::address_v6::bytes_type bytes;
	std::copy(d.address().begin(), d.address().end(), bytes.begin());
	out << boost::asio::ip::address_v6(bytes) << ") : ";

	unsigned count = 0;
	for (auto& ep : d.endpointNames()) {
		if (count++)
			out << ", ";
		out << ep.name();
	}

	out << ";";
}

void ASTPrinter::printState(State& s)
{
	indent();

	out << "state " << s.name().name() << " {\n";
	_indent++;

	for (auto& v : s.variables()) {
		v->accept(*this);
		out << "\n";
	}

	if (s.variables().size())
		out << "\n";

	for (auto& o : s.onBlocks()) {
		o->accept(*this);
		out << "\n";
	}

	if (s.onBlocks().size())
		out << "\n";

	_indent--;

	indent();
	out << "}";
}

void ASTPrinter::printMachineBody(MachineBody& m)
{
	_indent++;

	for (auto& v : m.variables()) {
		out << "\n";
		v->accept(*this);
	}

	if (m.variables().size())
		out << "\n";

	for (auto& s : m.states()) {
		out << "\n";
		printState(s);
	}

	_indent--;
	out << "\n};";
}

void ASTPrinter::printClassParams(std::vector<std::unique_ptr<ClassParameter>>& params)
{
	unsigned count = 0;
	for (auto& p : params) {
		if (count++)
			out << ", ";
		switch (p->kind()) {
		case ClassParameter::Kind::Value: {
			auto& cpv = static_cast<CPValue&>(*p);
			printType(cpv.type(), cpv.typeSource());
			out << " ";
			break;
		}

		case ClassParameter::Kind::Device: out << "device "; break;
		case ClassParameter::Kind::Endpoint: out << "endpoint "; break;
		}
		out << p->name();
	}
}

void ASTPrinter::visit(MachineClass& m)
{
	out << "machine class " << m.name().name() << "(";
	printClassParams(m.parameters());
	out << ") {";
	printMachineBody(m);
}

void ASTPrinter::visit(MachineDefinition& m)
{
	out << "machine " << m.name().name() << " {";
	printMachineBody(m);
}

void ASTPrinter::visit(MachineInstantiation& m)
{
	out << "machine " << m.name().name() << " : " << m.instanceOf().name() << "(";

	unsigned count = 0;
	for (auto& arg : m.arguments()) {
		if (count++)
			out << ", ";
		arg->accept(*this);
	}

	out << ");";
}

void ASTPrinter::printBehaviourBody(Behaviour& b)
{
	_indent++;

	for (auto& v : b.variables()) {
		out << "\n";
		v->accept(*this);
	}

	if (b.variables().size())
		out << "\n";

	for (auto& ep : b.endpoints()) {
		out << '\n';
		indent();
		out << "endpoint " << ep->name().name() << " : ";
		printType(ep->type(), nullptr);
		out << " {\n";
		_indent++;
		if (ep->readBlock()) {
			indent();
			out << "read {";
			_indent++;
			for (auto& s : ep->readBlock()->statements()) {
				out << "\n";
				s->accept(*this);
			}
			out << "\n";
			ep->readValue()->accept(*this);
			_indent--;
			out << "\n}";
		}
		if (ep->write()) {
			indent();
			out << "write ";
			_skipIndent = true;
			ep->write()->accept(*this);
		}
		_indent--;
		out << '\n';
		indent();
		out << '}';
	}

	if (b.endpoints().size())
		out << "\n";

	for (auto& o : b.onBlocks()) {
		out << "\n";
		o->accept(*this);
	}

	_indent--;
	indent();
	out << "\n};";
}

void ASTPrinter::visit(BehaviourClass& b)
{
	out << "behaviour class " << b.name().name() << "(";
	printClassParams(b.parameters());
	out << ") {";
	printBehaviourBody(b);
}

void ASTPrinter::visit(BehaviourDefinition& b)
{
	out << "behaviour " << b.name().name() << " on " << b.device().name() << " {";
	printBehaviourBody(b);
}

void ASTPrinter::visit(BehaviourInstantiation& b)
{
	out << "behaviour " << b.name().name() << " on " << b.device().name() << ": " << b.instanceOf().name() << "(";

	unsigned count = 0;
	for (auto& arg : b.arguments()) {
		if (count++)
			out << ", ";
		arg->accept(*this);
	}

	out << ");";
}

void ASTPrinter::visit(IncludeLine& i)
{
	out << "// include \"" << i.file() << "\";";
	includeStack.push_back(&i);
}

void ASTPrinter::visit(TranslationUnit& t)
{
	auto bundle = [] (ProgramPart& a, ProgramPart& b) {
		return (dynamic_cast<Endpoint*>(&a) && dynamic_cast<Endpoint*>(&b))
			|| (dynamic_cast<Device*>(&a) && dynamic_cast<Device*>(&b))
			|| (dynamic_cast<MachineInstantiation*>(&a) && dynamic_cast<MachineInstantiation*>(&b));
	};

	ProgramPart* last = nullptr;
	for (auto& item : t.items()) {
		if (last)
			out << '\n';

		while (!includeStack.empty() && item->sloc().parent() != &includeStack.back()->sloc()) {
			out << "// end of " << includeStack.back()->file() << "\n";
			includeStack.pop_back();
		}

		if (last && !bundle(*last, *item))
			out << "\n";

		item->accept(*this);
		last = item.get();
	}

	out << "\n";
}

}
}
