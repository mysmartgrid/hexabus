#include "Lang/sema.hpp"

#include "Util/range.hpp"

#include <boost/format.hpp>

using boost::format;

namespace hbt {
namespace lang {


static std::string cptDeclStr(ClassParameter::Type t)
{
	switch (t) {
	case ClassParameter::Type::Value: return "value";
	case ClassParameter::Type::Device: return "device name";
	case ClassParameter::Type::Endpoint: return "endpoint name";
	default: throw "invalid class parameter type";
	}
}


void SemanticVisitor::ScopeStack::enter()
{
	stack.push_back({});
}

void SemanticVisitor::ScopeStack::leave()
{
	stack.pop_back();
}

SemanticVisitor::ScopeEntry* SemanticVisitor::ScopeStack::resolve(const std::string& ident)
{
	for (auto& scope : util::reverse_range(stack)) {
		auto it = scope.find(ident);

		if (it != scope.end())
			return &it->second;
	}

	return nullptr;
}

SemanticVisitor::ScopeEntry* SemanticVisitor::ScopeStack::insert(DeclarationStmt& decl)
{
	return &stack.back().insert({ decl.name().name(), { &decl } }).first->second;
}


static void printIncludeStack(std::ostream& out, const SourceLocation* sloc)
{
	if (!sloc)
		return;

	printIncludeStack(out, sloc->parent());
	out << "in file included from " << sloc->file() << ":" << sloc->line() << ":" << sloc->col() << ":\n";
}

void SemanticVisitor::errorAt(const SourceLocation& sloc, const std::string& msg)
{
	if (hadError)
		diagOut << "\n";

	printIncludeStack(diagOut, sloc.parent());

	diagOut << sloc.file() << ":" << sloc.line() << ":" << sloc.col() << ": error: " << msg;
	diagOut << "\n";

	hadError = true;
}

void SemanticVisitor::hintAt(const SourceLocation& sloc, const std::string& msg)
{
	diagOut << sloc.file() << ":" << sloc.line() << ":" << sloc.col() << ": " << msg;
	diagOut << "\n";
}


void SemanticVisitor::declareGlobalName(ProgramPart& p, const std::string& name)
{
	auto res = globalNames.insert({ name, &p });

	if (!res.second) {
		errorAt(p.sloc(), "redeclaration of '" + name + "'");
		hintAt(res.first->second->sloc(), "previously declared here");
	}
}

bool SemanticVisitor::inferClassParam(const std::string& name, const SourceLocation& at, ClassParameter::Type type)
{
	auto cpit = classParams.find(name);
	if (cpit != classParams.end()) {
		auto cp = cpit->second;

		if (cp->type() == type) {
			return true;
		} else if (cp->type() == ClassParameter::Type::Unknown) {
			cp->type(type);
			classParamInferenceSites.insert({ name, &at });
			return true;
		}

		errorAt(at, "class parameter '" + name + "' used in multiple contexts");
		hintAt(at, "used as " + cptDeclStr(type) + " here");
		hintAt(*classParamInferenceSites.at(name), "used as " + cptDeclStr(cp->type()) + " here");
		return true;
	}

	return false;
}

void SemanticVisitor::visit(IdentifierExpr& i)
{
	if (auto se = scopes.resolve(i.name())) {
		i.type(se->declaration->type());
		return;
	}

	if (!inferClassParam(i.name(), i.sloc(), ClassParameter::Type::Value)) {
		errorAt(i.sloc(), "use of undeclared identifier '" + i.name() + "'");
	} else {
		i.isConstant(true);

		auto it = classParamTypes.find(i.name());
		if (it != classParamTypes.end()) {
			i.isDependent(false);
			i.type(it->second);
		} else {
			i.isDependent(true);
		}
	}
}

void SemanticVisitor::visit(TypedLiteral<bool>& l)
{
	l.isConstant(true);
}

void SemanticVisitor::visit(TypedLiteral<uint8_t>& l)
{
	l.isConstant(true);
}

void SemanticVisitor::visit(TypedLiteral<uint32_t>& l)
{
	l.isConstant(true);
}

void SemanticVisitor::visit(TypedLiteral<uint64_t>& l)
{
	l.isConstant(true);
}

void SemanticVisitor::visit(TypedLiteral<float>& l)
{
	l.isConstant(true);
}

void SemanticVisitor::visit(CastExpr& c)
{
	c.expr().accept(*this);
	c.isConstant(c.expr().isConstant());
}

void SemanticVisitor::visit(UnaryExpr& u)
{
	u.expr().accept(*this);
	u.isConstant(u.expr().isConstant());
	u.isDependent(u.expr().isDependent());

	if (u.isDependent())
		return;

	if (u.expr().type() == Type::Float && u.op() == UnaryOperator::Negate)
		errorAt(u.sloc(), "invalid type 'float' in unary negation");

	if (u.op() == UnaryOperator::Not)
		u.type(Type::Bool);
	else
		u.type(u.expr().type());
}

void SemanticVisitor::visit(BinaryExpr& b)
{
	b.left().accept(*this);
	b.right().accept(*this);

	b.isConstant(b.left().isConstant() && b.right().isConstant());
	b.isDependent(b.left().isDependent() || b.right().isDependent());

	if (b.isDependent())
		return;

	b.type(commonType(b.left().type(), b.right().type()));

	switch (b.op()) {
	case BinaryOperator::Plus:
	case BinaryOperator::Minus:
	case BinaryOperator::Multiply:
	case BinaryOperator::Divide:
	case BinaryOperator::Modulo:
		break;

	case BinaryOperator::And:
	case BinaryOperator::Or:
	case BinaryOperator::Xor:
	case BinaryOperator::ShiftLeft:
	case BinaryOperator::ShiftRight:
		if (b.left().type() == Type::Float || b.right().type() == Type::Float)
			errorAt(b.left().sloc(), "invalid types in binary expression (" + typeName(b.left().type()) + " and " + 
				typeName(b.right().type()) + ")");
		break;

	case BinaryOperator::BoolAnd:
	case BinaryOperator::BoolOr:
		if (b.left().type() != Type::Bool || b.right().type() != Type::Bool)
			errorAt(b.left().sloc(), "invalid types in binary expression (" + typeName(b.left().type()) + " and " + 
				typeName(b.right().type()) + ")");
		b.type(Type::Bool);
		break;

	case BinaryOperator::Equals:
	case BinaryOperator::NotEquals:
	case BinaryOperator::LessThan:
	case BinaryOperator::LessOrEqual:
	case BinaryOperator::GreaterThan:
	case BinaryOperator::GreaterOrEqual:
		b.type(Type::Bool);
		break;

	default:
		throw "invalid binary operator";
	}
}

void SemanticVisitor::visit(ConditionalExpr& c)
{
	c.condition().accept(*this);
	c.ifTrue().accept(*this);
	c.ifFalse().accept(*this);

	c.isConstant(c.condition().isConstant() && c.ifTrue().isConstant() && c.ifFalse().isConstant());
	c.isDependent(c.condition().isDependent() || c.ifTrue().isDependent() || c.ifFalse().isDependent());

	if (!c.isDependent())
		c.type(commonType(c.ifTrue().type(), c.ifFalse().type()));
}

Endpoint* SemanticVisitor::checkEndpointExpr(EndpointExpr& e)
{
	if (e.type() != Type::Unknown)
		return dynamic_cast<Endpoint*>(globalNames[e.endpoint().name()]);

	Device* dev = nullptr;
	Endpoint* ep = nullptr;

	{
		auto devit = globalNames.find(e.device().name());
		if (devit != globalNames.end()) {
			dev = dynamic_cast<Device*>(devit->second);
			if (!dev) {
				errorAt(e.device().sloc(), "identifier '" + e.device().name() + "' does not name a device");
				hintAt(devit->second->sloc(), "previously declared here");
			}
		} else if (auto se = scopes.resolve(e.device().name())) {
			errorAt(e.device().sloc(), "identifier '" + e.device().name() + "' does not name a device");
			hintAt(se->declaration->sloc(), "previously declared here");
		} else if (!inferClassParam(e.device().name(), e.device().sloc(), ClassParameter::Type::Device)) {
			errorAt(e.device().sloc(), "use of undeclared device '" + e.device().name() + "'");
		}
	}

	{
		auto epit = globalNames.find(e.endpoint().name());
		if (epit != globalNames.end()) {
			ep = dynamic_cast<Endpoint*>(epit->second);
			if (!ep) {
				errorAt(e.endpoint().sloc(), "identifier '" + e.endpoint().name() + "' does not name an endpoint");
				hintAt(epit->second->sloc(), "previously declared here");
			}
		} else if (auto se = scopes.resolve(e.endpoint().name())) {
			errorAt(e.endpoint().sloc(), "identifier '" + e.endpoint().name() + "' does not name an endpoint");
			hintAt(se->declaration->sloc(), "previously declared here");
		} else if (!inferClassParam(e.endpoint().name(), e.endpoint().sloc(), ClassParameter::Type::Endpoint)) {
			errorAt(e.endpoint().sloc(), "use of undeclared endpoint '" + e.endpoint().name() + "'");
		}
	}

	if (!dev || !ep)
		return nullptr;

	bool hasEP = dev->endpoints().end() != std::find_if(dev->endpoints().begin(), dev->endpoints().end(),
		[ep] (const Identifier& i) {
			return i.name() == ep->name().name();
		});
	if (!hasEP) {
		errorAt(e.sloc(), "device '" + dev->name().name() + "' does not implement endpoint '" + ep->name().name() + "'");
		return nullptr;
	}

	e.type(ep->type());

	return ep;
}

void SemanticVisitor::visit(EndpointExpr& e)
{
	if (auto ep = checkEndpointExpr(e)) {
		if ((ep->access() & EndpointAccess::Read) != EndpointAccess::Read)
			errorAt(e.sloc(), "endpoint '" + e.endpoint().name() + "' cannot be read");
	}
}

void SemanticVisitor::visit(CallExpr& c)
{
	for (auto& arg : c.arguments())
		arg->accept(*this);

	if (c.name() != "hour" && c.name() != "minute" && c.name() != "second" &&
			c.name() != "day" && c.name() != "month" && c.name() != "year" && c.name() != "weekday") {
		errorAt(c.sloc(), "use of undeclared identifier '" + c.name() + "'");
		return;
	}

	if (c.arguments().size() != 1)
		errorAt(c.sloc(), str(format("function '%1%' expects 1 arguments, %2% given") % c.name() % c.arguments().size()));

	c.type(Type::UInt32);
	c.isConstant(c.arguments()[0]->isConstant());
	c.isDependent(c.arguments()[0]->isDependent());
}

void SemanticVisitor::visit(PacketEIDExpr& p)
{
	// nothing to do
}

void SemanticVisitor::visit(TimeoutExpr& t)
{
	// nothing to do
}

void SemanticVisitor::visit(AssignStmt& a)
{
	ScopeEntry* se = scopes.resolve(a.target().name());

	a.value().accept(*this);

	if (!se) {
		errorAt(a.sloc(), "use of undeclared identifier '" + a.target().name() + "'");
		return;
	}

	if (!a.value().isDependent() && !isAssignableFrom(se->declaration->type(), a.value().type())) {
		errorAt(a.sloc(), str(
			format("invalid implicit type conversion from %1% to %2%")
				% typeName(a.value().type())
				% typeName(se->declaration->type())));
		return;
	}
}

void SemanticVisitor::visit(WriteStmt& w)
{
	w.value().accept(*this);

	if (auto ep = checkEndpointExpr(w.target())) {
		if ((ep->access() & EndpointAccess::Write) != EndpointAccess::Write)
			errorAt(w.sloc(), "endpoint '" + w.target().endpoint().name() + "' cannot be written");

		if (!w.value().isDependent() && ep->type() != w.value().type())
			errorAt(w.sloc(), "invalid types in assignment (" + typeName(ep->type()) + " and " +
				typeName(w.value().type()) + ")");
	}
}

void SemanticVisitor::visit(IfStmt& i)
{
	i.condition().accept(*this);
	i.ifTrue().accept(*this);
	if (i.ifFalse())
		i.ifFalse()->accept(*this);
}

void SemanticVisitor::visit(SwitchStmt& s)
{
	s.expr().accept(*this);
	if (!s.expr().isDependent() && s.expr().type() == Type::Float)
		errorAt(s.expr().sloc(), "cannot switch on type float");

	const SourceLocation* defaultPos = nullptr;
	for (auto& se : s.entries()) {
		scopes.enter();

		for (auto& l : se.labels()) {
			if (l.expr()) {
				l.expr()->accept(*this);
				if (!l.expr()->isDependent()) {
					if (!l.expr()->isConstant() || l.expr()->type() == Type::Float)
						errorAt(l.sloc(), "case labels must be integral constant expressions");
				}
			} else if (!l.expr()) {
				if (defaultPos) {
					errorAt(l.sloc(), "duplicate default labels");
					hintAt(*defaultPos, "first set here");
				}
				defaultPos = &l.sloc();
			}
		}

		se.statement().accept(*this);

		scopes.leave();
	}
}

void SemanticVisitor::visit(BlockStmt& b)
{
	scopes.enter();
	for (auto& s : b.statements())
		s->accept(*this);
	scopes.leave();
}

void SemanticVisitor::visit(DeclarationStmt& d)
{
	ScopeEntry* decl = nullptr;

	if (auto old = scopes.resolve(d.name().name())) {
		errorAt(d.sloc(), "redeclaration of '" + d.name().name() + "'");
		hintAt(old->declaration->sloc(), "previously declared here");
	} else {
		decl = scopes.insert(d);
	}

	if (d.value()) {
		d.value()->accept(*this);
		if (decl) {
			if (!isAssignableFrom(d.type(), d.value()->type()))
				errorAt(d.sloc(), str(
					format("invalid implicit type conversion from %1% to %2%")
						% typeName(d.value()->type())
						% typeName(d.type())));
		}
	}
}

void SemanticVisitor::visit(GotoStmt& g)
{
	if (!knownStates.count(g.state().name()))
		errorAt(g.state().sloc(), "use of undeclared state '" + g.state().name() + "'");
}

void SemanticVisitor::visit(OnSimpleBlock& o)
{
	o.block().accept(*this);
}

void SemanticVisitor::visit(OnPacketBlock& o)
{
	auto it = globalNames.find(o.source().name());

	if (it == globalNames.end())
		errorAt(o.sloc(), "use of undeclared device '" + o.source().name() + "'");
	else if (!dynamic_cast<Device*>(it->second))
		errorAt(o.sloc(), "identifier '" + o.source().name() + "' does not name a device");

	o.block().accept(*this);
}

void SemanticVisitor::visit(OnExprBlock& o)
{
	o.condition().accept(*this);

	if (!o.condition().isDependent() && o.condition().type() != Type::Bool) {
		errorAt(o.sloc(), str(
			format("invalid implicit type conversion from %1% to %2%")
				% typeName(o.condition().type())
				% typeName(Type::Bool)));
	}

	o.block().accept(*this);
}

void SemanticVisitor::visit(Endpoint& e)
{
	declareGlobalName(e, e.name().name());

	auto res = endpointsByEID.insert({ e.eid(), &e });

	if (!res.second) {
		errorAt(e.sloc(), str(format("redefinition of EID %1%") % e.eid()));
		hintAt(res.first->second->sloc(), "previously defined here");
	}
}

void SemanticVisitor::visit(Device& d)
{
	declareGlobalName(d, d.name().name());

	for (auto& ep : d.endpoints()) {
		auto eit = globalNames.find(ep.name());

		if (eit == globalNames.end()) {
			errorAt(ep.sloc(), "undefined endpoint '" + ep.name() + "'");
			continue;
		}

		auto epDef = dynamic_cast<Endpoint*>(eit->second);
		if (!epDef) {
			errorAt(ep.sloc(), "'" + ep.name() + "' is not an endpoint");
			hintAt(eit->second->sloc(), "declared here");
		}
	}
}

void SemanticVisitor::checkState(State& s)
{
	scopes.enter();
	for (auto& var : s.variables())
		var.accept(*this);
	for (auto& on : s.onBlocks())
		on->accept(*this);
	for (auto& stmt : s.statements())
		stmt->accept(*this);
	scopes.leave();
}

void SemanticVisitor::checkMachineBody(MachineBody& m)
{
	knownStates.clear();

	for (auto& state : m.states()) {
		auto res = knownStates.insert({ state.name().name(), &state });

		if (!res.second) {
			errorAt(state.sloc(), "redefinition of '" + state.name().name() + "'");
			hintAt(res.first->second->sloc(), "previously defined here");
		}
	}

	scopes.enter();
	for (auto& var : m.variables())
		var.accept(*this);
	for (auto& state : m.states())
		checkState(state);
	scopes.leave();
}

void SemanticVisitor::visit(MachineClass& m)
{
	declareGlobalName(m, m.name().name());

	for (auto& param : m.parameters()) {
		auto res = classParams.insert({ param.name(), &param });

		if (!res.second) {
			errorAt(param.sloc(), "redeclaration of clas parameter '" + param.name() + "'");
			hintAt(res.first->second->sloc(), "previously declared here");
		}
	}

	checkMachineBody(m);
	classParams.clear();
	classParamInferenceSites.clear();
}

void SemanticVisitor::visit(MachineDefinition& m)
{
	declareGlobalName(m, m.name().name());
	checkMachineBody(m);
}

void SemanticVisitor::visit(MachineInstantiation& m)
{
	declareGlobalName(m, m.name().name());

	auto mit = globalNames.find(m.instanceOf().name());
	if (mit == globalNames.end()) {
		errorAt(m.instanceOf().sloc(), "undeclared identifier " + m.instanceOf().name());
		return;
	}

	auto mclass = dynamic_cast<MachineClass*>(mit->second);
	if (!mclass) {
		errorAt(m.instanceOf().sloc(), "'" + m.instanceOf().name() + "' is not a machine class");
		hintAt(mit->second->sloc(), "declared here");
		return;
	}

	if (m.arguments().size() != mclass->parameters().size()) {
		errorAt(m.sloc(),
			str(format("%1% expects %2% parameters, %3% given")
				% mclass->name().name()
				% mclass->parameters().size()
				% m.arguments().size()));
	} else {
		auto arg = m.arguments().begin(), aend = m.arguments().end();
		auto param = mclass->parameters().begin(), pend = mclass->parameters().end();

		for (; arg != aend; ++arg, ++param) {
			switch (param->type()) {
			case ClassParameter::Type::Value:
				(*arg)->accept(*this);
				if (!(*arg)->isConstant())
					errorAt((*arg)->sloc(), "expected constant value for class parameter '" + param->name() + "'");
				classParamTypes.insert({ param->name(), (*arg)->type() });
				break;

			case ClassParameter::Type::Device:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto dev = globalNames.find(id->name());
					if (dev == globalNames.end() || !dynamic_cast<Device*>(dev->second))
						errorAt(id->sloc(), "identifier '" + id->name() + "' does not name a device");
				} else {
					errorAt((*arg)->sloc(), "expected device identifier for class parameter '" + param->name() + "'");
				}
				break;

			case ClassParameter::Type::Endpoint:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto ep = globalNames.find(id->name());
					if (ep == globalNames.end() || !dynamic_cast<Endpoint*>(ep->second))
						errorAt(id->sloc(), "identifier '" + id->name() + "' does not name an endpoint");
				} else {
					errorAt((*arg)->sloc(), "expected endpoint identifier for class parameter '" + param->name() + "'");
				}
				break;

			default:
				throw "invalid class parameter type";
			}

			classParams.insert({ param->name(), &*param });
		}

		checkMachineBody(*mclass);
		classParams.clear();
		classParamTypes.clear();
	}
}

void SemanticVisitor::visit(IncludeLine& i)
{
	// nothing to do
}

void SemanticVisitor::visit(TranslationUnit& t)
{
	hadError = false;
	globalNames.clear();
	endpointsByEID.clear();
	scopes = ScopeStack();

	for (auto& item : t.items())
		item->accept(*this);
}

}
}
