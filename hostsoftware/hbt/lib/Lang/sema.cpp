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

static Diagnostic redeclaration(const SourceLocation& sloc, const std::string& name)
{
	return { DiagnosticKind::Error, &sloc, str(format("redeclaration of '%1%'") % name) };
}

static Diagnostic previouslyDeclaredHere(const SourceLocation& sloc)
{
	return { DiagnosticKind::Hint, &sloc, "previously declared here" };
}

static Diagnostic declaredHere(const SourceLocation& sloc)
{
	return { DiagnosticKind::Hint, &sloc, "declared here" };
}

static Diagnostic classParameterTypeError(const SourceLocation& sloc, const std::string& name)
{
	return { DiagnosticKind::Error, &sloc, str(format("class parameter '%1%' used in incompatible contexts") % name) };
}

static Diagnostic cptUsedAs(const SourceLocation& sloc, ClassParameter::Type type)
{
	return { DiagnosticKind::Hint, &sloc, str(format("used as %1% here") % cptDeclStr(type)) };
}

static Diagnostic undeclaredIdentifier(const Identifier& id)
{
	return { DiagnosticKind::Error, &id.sloc(), str(format("use of undeclared identifier '%1%'") % id.name()) };
}

static Diagnostic invalidUnaryOp(const SourceLocation& sloc, Type type)
{
	return { DiagnosticKind::Error, &sloc, str(format("invalid type %1% in unary expression") % typeName(type)) };
}

static Diagnostic invalidBinaryOp(const SourceLocation& sloc, Type left, Type right)
{
	return {
		DiagnosticKind::Error,
		&sloc,
		str(format("invalid types in binary expression (%1% and %2%)") % typeName(left) % typeName(right))
	};
}

static Diagnostic identifierIsNoDevice(const Identifier& ident)
{
	return { DiagnosticKind::Error, &ident.sloc(), str(format("identifier '%1%' does not name a device") % ident.name()) };
}

static Diagnostic identifierIsNoEndpoint(const Identifier& ident)
{
	return { DiagnosticKind::Error, &ident.sloc(), str(format("identifier '%1%' does not name an endpoint") % ident.name()) };
}

static Diagnostic identifierIsNoClass(const Identifier& ident)
{
	return { DiagnosticKind::Error, &ident.sloc(), str(format("identifier '%1%' does not name a machine class") % ident.name()) };
}

static Diagnostic deviceDoesNotHave(const EndpointExpr& e, const std::string& dev, const std::string& ep)
{
	auto& devName = e.deviceIsDependent()
		? str(format("%1% (aka %2%)") % e.device().name() % dev)
		: e.device().name();
	auto& epName = e.endpointIsDependent()
		? str(format("%1% (aka %2%)") % e.endpoint().name() % ep)
		: e.endpoint().name();
	return { DiagnosticKind::Error, &e.sloc(), str(format("device %1% does not implement endpoint %2%") % devName % epName) };
}

static Diagnostic endpointNotReadable(const EndpointExpr& ep, const Endpoint& subst)
{
	if (ep.endpointIsDependent()) {
		return {
			DiagnosticKind::Error,
			&ep.sloc(),
			str(format("endpoint %1% (aka %2%) cannot be read") % ep.endpoint().name() % subst.name().name()) };
	} else {
		return { DiagnosticKind::Error, &ep.sloc(), str(format("endpoint %1% cannot be read") % ep.endpoint().name()) };
	}
}

static Diagnostic endpointNotWritable(const EndpointExpr& ep, const Endpoint& subst)
{
	if (ep.endpointIsDependent()) {
		return {
			DiagnosticKind::Error,
			&ep.sloc(),
			str(format("endpoint %1% (aka %2%) cannot be written") % ep.endpoint().name() % subst.name().name()) };
	} else {
		return { DiagnosticKind::Error, &ep.sloc(), str(format("endpoint %1% cannot be written") % ep.endpoint().name()) };
	}
}

static Diagnostic invalidCallArgCount(CallExpr& c, unsigned expected)
{
	return {
		DiagnosticKind::Error,
		&c.sloc(),
		str(format("too %1% arguments to call of function %2% (expected %3%, got %4%)")
			% (c.arguments().size() < expected ? "few" : "many")
			% c.name().name()
			% expected
			% c.arguments().size())
	};
}

static Diagnostic invalidArgType(CallExpr& c, Type expected, unsigned arg)
{
	return {
		DiagnosticKind::Error,
		&c.sloc(),
		str(format("invalid type for argument %1% of function %2% (expected %3%, got %4%)")
			% (arg + 1)
			% c.name().name()
			% typeName(expected)
			% typeName(c.arguments()[arg]->type()))
	};
}

static Diagnostic invalidImplicitConversion(const SourceLocation& sloc, Type from, Type to)
{
	return {
		DiagnosticKind::Error,
		&sloc,
		str(format("invalid implicit type conversion from %1% to %2%") % typeName(from) % typeName(to))
	};
}

static Diagnostic caseLabelInvalid(const SwitchLabel& l)
{
	return { DiagnosticKind::Error, &l.sloc(), "case label must be an integral constant expression" };
}

static Diagnostic caseLabelDuplicated(const SwitchLabel& l)
{
	return { DiagnosticKind::Error, &l.sloc(), "duplicated case label value" };
}

static Diagnostic eidRedefined(const Endpoint& ep)
{
	return { DiagnosticKind::Error, &ep.sloc(), str(format("redefinition of endpoint %1%") % ep.eid()) };
}

static Diagnostic stateRedefined(const State& s)
{
	return { DiagnosticKind::Error, &s.sloc(), str(format("redefinition of state %1%") % s.name().name()) };
}

static Diagnostic classParamRedefined(const ClassParameter& cp)
{
	return { DiagnosticKind::Error, &cp.sloc(), str(format("redefinition of class parameter %1%") % cp.name()) };
}

static Diagnostic invalidClassArgCount(MachineInstantiation& m, unsigned expected)
{
	return {
		DiagnosticKind::Error,
		&m.sloc(),
		str(format("too %1% arguments in instantiation of class %2% (expected %3%, got %4%)")
			% (m.arguments().size() < expected ? "few" : "many")
			% m.instanceOf().name()
			% expected
			% m.arguments().size())
	};
}

static Diagnostic classArgumentInvalid(const SourceLocation& sloc, const ClassParameter& cp)
{
	return {
		DiagnosticKind::Error,
		&sloc,
		str(format("expected %1% for class parameter %2%")
			% cptDeclStr(cp.type())
			% cp.name())
	};
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



void SemanticVisitor::declareGlobalName(ProgramPart& p, const std::string& name)
{
	auto res = globalNames.insert({ name, &p });

	if (!res.second)
		diags.print(
			redeclaration(p.sloc(), name),
			previouslyDeclaredHere(res.first->second->sloc()));
}

bool SemanticVisitor::inferClassParam(const std::string& name, const SourceLocation& at, ClassParameter::Type type)
{
	auto cpit = classParams.find(name);
	if (cpit != classParams.end()) {
		auto& cp = cpit->second;

		if (cp.parameter.type() == type) {
			return true;
		} else if (cp.parameter.type() == ClassParameter::Type::Unknown) {
			cp.parameter.type(type);
			cp.inferredFrom = &at;
			return true;
		}

		diags.print(
			classParameterTypeError(at, name),
			cptUsedAs(at, type),
			cptUsedAs(*cp.inferredFrom, cp.parameter.type()));
	}

	return false;
}

void SemanticVisitor::visit(IdentifierExpr& i)
{
	if (auto se = scopes.resolve(i.name())) {
		i.type(se->declaration->type());
		return;
	}

	if (inferClassParam(i.name(), i.sloc(), ClassParameter::Type::Value)) {
		i.isConstant(true);

		auto& cp = classParams.at(i.name());
		if (cp.hasValue) {
			i.isDependent(false);
			i.type(cp.value->type());
		} else {
			i.isDependent(true);
		}
		return;
	}

	if (!classParams.count(i.name()))
		diags.print(undeclaredIdentifier(Identifier(i.sloc(), i.name())));
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
		diags.print(invalidUnaryOp(u.sloc(), u.expr().type()));

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
			diags.print(invalidBinaryOp(b.sloc(), b.left().type(), b.right().type()));
		break;

	case BinaryOperator::BoolAnd:
	case BinaryOperator::BoolOr:
		if (b.left().type() != Type::Bool || b.right().type() != Type::Bool)
			diags.print(invalidBinaryOp(b.sloc(), b.left().type(), b.right().type()));
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
	bool exprIsFullyDefined = true;

	{
		auto cpit = classParams.find(e.device().name());
		auto it = globalNames.find(e.device().name());

		if (auto se = scopes.resolve(e.device().name())) {
			diags.print(
				identifierIsNoDevice(e.device()),
				previouslyDeclaredHere(se->declaration->sloc()));
		} else if (cpit != classParams.end()) {
			exprIsFullyDefined &= cpit->second.hasValue;
			if (inferClassParam(e.device().name(), e.device().sloc(), ClassParameter::Type::Device) && cpit->second.hasValue)
				dev = cpit->second.device;
			e.deviceIsDependent(!dev);
		} else if (it != globalNames.end()) {
			dev = dynamic_cast<Device*>(it->second);
			if (!dev)
				diags.print(
					identifierIsNoDevice(e.device()),
					declaredHere(it->second->sloc()));
		} else {
			diags.print(undeclaredIdentifier(e.device()));
		}
	}

	{
		auto cpit = classParams.find(e.endpoint().name());
		auto it = globalNames.find(e.endpoint().name());

		if (auto se = scopes.resolve(e.endpoint().name())) {
			diags.print(
				identifierIsNoDevice(e.endpoint()),
				declaredHere(se->declaration->sloc()));
		} else if (cpit != classParams.end()) {
			exprIsFullyDefined &= cpit->second.hasValue;
			if (inferClassParam(e.endpoint().name(), e.endpoint().sloc(), ClassParameter::Type::Endpoint) && cpit->second.hasValue)
				ep = cpit->second.endpoint;
			e.deviceIsDependent(!ep);
		} else if (it != globalNames.end()) {
			ep = dynamic_cast<Endpoint*>(it->second);
			if (!ep)
				diags.print(
					identifierIsNoEndpoint(e.endpoint()),
					previouslyDeclaredHere(it->second->sloc()));
		} else {
			diags.print(undeclaredIdentifier(e.endpoint()));
		}
	}

	if (!dev || !ep)
		return nullptr;

	if (!exprIsFullyDefined)
		return nullptr;

	bool hasEP = dev->endpoints().end() != std::find_if(dev->endpoints().begin(), dev->endpoints().end(),
		[ep] (const Identifier& i) {
			return i.name() == ep->name().name();
		});
	if (!hasEP) {
		diags.print(deviceDoesNotHave(e, dev->name().name(), ep->name().name()));
		return nullptr;
	}

	e.type(ep->type());

	return ep;
}

void SemanticVisitor::visit(EndpointExpr& e)
{
	if (auto ep = checkEndpointExpr(e)) {
		if ((ep->access() & EndpointAccess::Broadcast) != EndpointAccess::Broadcast)
			diags.print(endpointNotReadable(e, *ep));
	}
}

void SemanticVisitor::visit(CallExpr& c)
{
	for (auto& arg : c.arguments())
		arg->accept(*this);

	if (c.name().name() != "hour" && c.name().name() != "minute" && c.name().name() != "second" &&
			c.name().name() != "day" && c.name().name() != "month" && c.name().name() != "year" &&
			c.name().name() != "weekday") {
		diags.print(undeclaredIdentifier(c.name()));
		return;
	}

	c.type(Type::UInt32);

	if (c.arguments().size() != 1)
		diags.print(invalidCallArgCount(c, 1));

	if (c.arguments().size() < 1)
		return;

	if (!isIntType(c.arguments()[0]->type()))
		diags.print(invalidArgType(c, Type::UInt64, 0));

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
		diags.print(undeclaredIdentifier(a.target()));
		return;
	}

	if (!a.value().isDependent() && !isAssignableFrom(se->declaration->type(), a.value().type()))
		diags.print(invalidImplicitConversion(a.sloc(), a.value().type(), se->declaration->type()));
}

void SemanticVisitor::visit(WriteStmt& w)
{
	w.value().accept(*this);

	if (auto ep = checkEndpointExpr(w.target())) {
		if ((ep->access() & EndpointAccess::Write) != EndpointAccess::Write)
			diags.print(endpointNotWritable(w.target(), *ep));

		if (!w.value().isDependent() && ep->type() != w.value().type())
			diags.print(invalidImplicitConversion(w.sloc(), w.value().type(), ep->type()));
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
		diags.print(invalidImplicitConversion(s.sloc(), s.expr().type(), Type::UInt32));

	const SwitchLabel* defaultPos = nullptr;
	for (auto& se : s.entries()) {
		scopes.enter();

		for (auto& l : se.labels()) {
			if (l.expr()) {
				l.expr()->accept(*this);
				if (!l.expr()->isDependent()) {
					if (!l.expr()->isConstant() || l.expr()->type() == Type::Float)
						diags.print(caseLabelInvalid(l));
				}
			} else if (!l.expr()) {
				if (defaultPos)
					diags.print(
						caseLabelInvalid(l),
						previouslyDeclaredHere(defaultPos->sloc()));

				defaultPos = &l;
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
		diags.print(
			redeclaration(d.name().sloc(), d.name().name()),
			previouslyDeclaredHere(old->declaration->sloc()));
	} else {
		decl = scopes.insert(d);
	}

	d.value().accept(*this);
	if (decl && !d.value().isDependent() && !isAssignableFrom(d.type(), d.value().type()))
		diags.print(invalidImplicitConversion(d.sloc(), d.value().type(), d.type()));
}

void SemanticVisitor::visit(GotoStmt& g)
{
	if (!knownStates.count(g.state().name()))
		diags.print(undeclaredIdentifier(g.state()));
}

void SemanticVisitor::visit(OnSimpleBlock& o)
{
	o.block().accept(*this);
}

void SemanticVisitor::visit(OnPacketBlock& o)
{
	auto it = globalNames.find(o.source().name());

	if (it == globalNames.end())
		diags.print(undeclaredIdentifier(o.source()));
	else if (!dynamic_cast<Device*>(it->second))
		diags.print(identifierIsNoDevice(o.source()));

	o.block().accept(*this);
}

void SemanticVisitor::visit(OnExprBlock& o)
{
	o.condition().accept(*this);

	if (!o.condition().isDependent() && o.condition().type() != Type::Bool)
		diags.print(invalidImplicitConversion(o.sloc(), o.condition().type(), Type::Bool));

	o.block().accept(*this);
}

void SemanticVisitor::visit(Endpoint& e)
{
	declareGlobalName(e, e.name().name());

	auto res = endpointsByEID.insert({ e.eid(), &e });

	if (!res.second)
		diags.print(
			eidRedefined(e),
			previouslyDeclaredHere(res.first->second->sloc()));
}

void SemanticVisitor::visit(Device& d)
{
	declareGlobalName(d, d.name().name());

	for (auto& ep : d.endpoints()) {
		auto eit = globalNames.find(ep.name());

		if (eit == globalNames.end()) {
			diags.print(undeclaredIdentifier(ep));
			continue;
		}

		auto epDef = dynamic_cast<Endpoint*>(eit->second);
		if (!epDef)
			diags.print(
				identifierIsNoEndpoint(Identifier(ep.sloc(), ep.name())),
				declaredHere(eit->second->sloc()));
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

		if (!res.second)
			diags.print(
				stateRedefined(state),
				previouslyDeclaredHere(res.first->second->sloc()));
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
		auto res = classParams.insert({ param.name(), { param, nullptr } });

		if (!res.second)
			diags.print(
				classParamRedefined(param),
				previouslyDeclaredHere(res.first->second.parameter.sloc()));
	}

	checkMachineBody(m);
	classParams.clear();
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
		diags.print(undeclaredIdentifier(m.instanceOf()));
		return;
	}

	auto mclass = dynamic_cast<MachineClass*>(mit->second);
	if (!mclass) {
		diags.print(
			identifierIsNoClass(m.instanceOf()),
			declaredHere(mit->second->sloc()));
		return;
	}

	if (m.arguments().size() != mclass->parameters().size()) {
		diags.print(invalidClassArgCount(m, mclass->parameters().size()));
	} else {
		auto arg = m.arguments().begin(), aend = m.arguments().end();
		auto param = mclass->parameters().begin(), pend = mclass->parameters().end();

		for (; arg != aend; ++arg, ++param) {
			switch (param->type()) {
			case ClassParameter::Type::Value:
				(*arg)->accept(*this);
				if (!(*arg)->isConstant())
					diags.print(classArgumentInvalid((*arg)->sloc(), *param));
				else
					classParams.insert({ param->name(), { *param, nullptr, (*arg).get() } });
				break;

			case ClassParameter::Type::Device:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto dev = globalNames.find(id->name());
					if (dev == globalNames.end())
						diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
					else if (auto decl = dynamic_cast<Device*>(dev->second))
						classParams.insert({ param->name(), { *param, nullptr, decl } });
					else
						diags.print(identifierIsNoDevice(Identifier(id->sloc(), id->name())));
				} else {
					diags.print(classArgumentInvalid((*arg)->sloc(), *param));
				}
				break;

			case ClassParameter::Type::Endpoint:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto ep = globalNames.find(id->name());
					if (ep == globalNames.end())
						diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
					else if (auto decl = dynamic_cast<Endpoint*>(ep->second))
						classParams.insert({ param->name(), { *param, nullptr, decl } });
					else
						diags.print(identifierIsNoEndpoint(Identifier(id->sloc(), id->name())));
				} else {
					diags.print(classArgumentInvalid((*arg)->sloc(), *param));
				}
				break;

			default:
				throw "invalid class parameter type";
			}
		}

		auto hint = diags.useHint(m.sloc(), "instantiated from here");

		if (classParams.size() == mclass->parameters().size())
			checkMachineBody(*mclass);
		classParams.clear();
	}
}

void SemanticVisitor::visit(IncludeLine& i)
{
	// nothing to do
}

void SemanticVisitor::visit(TranslationUnit& t)
{
	globalNames.clear();
	endpointsByEID.clear();
	scopes = ScopeStack();

	for (auto& item : t.items())
		item->accept(*this);
}

}
}
