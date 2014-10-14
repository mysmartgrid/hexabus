#include "Lang/sema.hpp"

#include "Util/range.hpp"

#include <boost/format.hpp>
#include <cln/integer_io.h>

using boost::format;

namespace hbt {
namespace lang {

static std::string cptDeclStr(ClassParameter::Type t)
{
	switch (t) {
	case ClassParameter::Type::Value: return "integer constant expression";
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
	auto& devName = e.deviceIdIsIncomplete()
		? str(format("%1% (aka %2%)") % e.deviceId().name() % dev)
		: e.deviceId().name();
	auto& epName = e.endpointIdIsIncomplete()
		? str(format("%1% (aka %2%)") % e.endpointId().name() % ep)
		: e.endpointId().name();
	return { DiagnosticKind::Error, &e.sloc(), str(format("device %1% does not implement endpoint %2%") % devName % epName) };
}

static Diagnostic endpointNotReadable(const EndpointExpr& ep, const Endpoint& subst)
{
	if (ep.endpointIdIsIncomplete()) {
		return {
			DiagnosticKind::Error,
			&ep.sloc(),
			str(format("endpoint %1% (aka %2%) cannot be read") % ep.endpointId().name() % subst.name().name()) };
	} else {
		return { DiagnosticKind::Error, &ep.sloc(), str(format("endpoint %1% cannot be read") % ep.endpointId().name()) };
	}
}

static Diagnostic endpointNotWritable(const EndpointExpr& ep, const Endpoint& subst)
{
	if (ep.endpointIdIsIncomplete()) {
		return {
			DiagnosticKind::Error,
			&ep.sloc(),
			str(format("endpoint %1% (aka %2%) cannot be written") % ep.endpointId().name() % subst.name().name()) };
	} else {
		return { DiagnosticKind::Error, &ep.sloc(), str(format("endpoint %1% cannot be written") % ep.endpointId().name()) };
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
	return { DiagnosticKind::Error, &l.sloc(), "duplicated case label 'default'" };
}

static Diagnostic caseLabelDuplicated(const SwitchLabel& l, const cln::cl_I& value)
{
	return { DiagnosticKind::Error, &l.sloc(), str(format("duplicated case label value %1%") % value) };
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

static Diagnostic constexprInvokesUB(const Expr& b)
{
	return { DiagnosticKind::Error, &b.sloc(), "evaluation of constant expression invokes undefined behaviour" };
}

static Diagnostic classParamUnused(const ClassParameter& cp)
{
	return { DiagnosticKind::Warning, &cp.sloc(), "unused class parameter " + cp.name() };
}

static Diagnostic classWithoutParameters(const MachineClass& m)
{
	return { DiagnosticKind::Error, &m.sloc(), str(format("class %1% has no parameters") % m.name().name()) };
}

static Diagnostic constexprDoesNotFit(const Expr& e)
{
	return {
		DiagnosticKind::Error,
		&e.sloc(),
		str(format("value %1% overflows type %2%") % e.constexprValue() % typeName(e.type()))
	};
}

static Diagnostic expectedIntType(const Expr& e)
{
	return {
		DiagnosticKind::Error,
		&e.sloc(),
		str(format("expected expression of integral type, got %1%") % typeName(e.type()))
	};
}

static Diagnostic invalidShiftAmount(BinaryExpr& b)
{
	return {
		DiagnosticKind::Error,
		&b.sloc(),
		str(format("invalid shift of type %1% by %2% bits") % typeName(b.left().type()) % b.right().constexprValue())
	};
}

static Diagnostic identifierIsFunction(const SourceLocation& sloc, const std::string& name)
{
	return { DiagnosticKind::Error, &sloc, str(format("'%1%' names a function") % name) };
}



template<typename T>
static bool fitsIntoNumericLimits(const cln::cl_I& value)
{
	typedef typename std::conditional<std::is_signed<T>::value, int64_t, uint64_t>::type clt;
	return clt(std::numeric_limits<T>::min()) <= value && value <= clt(std::numeric_limits<T>::max());
}

static bool fitsIntoConstexprType(Expr& e, Type  t)
{
	switch (t) {
	case Type::Bool: return fitsIntoNumericLimits<bool>(e.constexprValue()); break;
	case Type::UInt8: return fitsIntoNumericLimits<uint8_t>(e.constexprValue()); break;
	case Type::UInt16: return fitsIntoNumericLimits<uint16_t>(e.constexprValue()); break;
	case Type::UInt32: return fitsIntoNumericLimits<uint32_t>(e.constexprValue()); break;
	case Type::UInt64: return fitsIntoNumericLimits<uint64_t>(e.constexprValue()); break;
	case Type::Int8: return fitsIntoNumericLimits<int8_t>(e.constexprValue()); break;
	case Type::Int16: return fitsIntoNumericLimits<int16_t>(e.constexprValue()); break;
	case Type::Int32: return fitsIntoNumericLimits<int32_t>(e.constexprValue()); break;
	case Type::Int64: return fitsIntoNumericLimits<int64_t>(e.constexprValue()); break;
	default: throw "invalid type";
	}
}

static bool fitsIntoConstexprType(Expr& e)
{
	return fitsIntoConstexprType(e, e.type());
}

static bool isContextuallyConvertibleTo(Expr& e, Type t)
{
	if (e.isConstexpr())
		return isAssignableFrom(t, e.type()) || (fitsIntoConstexprType(e, t) && isIntType(e.type()) == isIntType(t));
	else
		return isAssignableFrom(t, e.type());
}

static bool isFunctionName(const std::string& name)
{
	return
		name == "second"
		|| name == "minute"
		|| name == "hour"
		|| name == "day"
		|| name == "month"
		|| name == "year"
		|| name == "weekday";
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
	if (isFunctionName(name)) {
		diags.print(identifierIsFunction(p.sloc(), name));
		return;
	}

	auto res = globalNames.insert({ name, &p });

	if (!res.second)
		diags.print(
			redeclaration(p.sloc(), name),
			previouslyDeclaredHere(res.first->second->sloc()));
}

void SemanticVisitor::visit(IdentifierExpr& i)
{
	if (auto se = scopes.resolve(i.name())) {
		i.type(se->declaration->type());
		return;
	}

	i.isIncomplete(true);

	if (isFunctionName(i.name())) {
		diags.print(identifierIsFunction(i.sloc(), i.name()));
		return;
	}

	auto cpit = classParams.find(i.name());
	if (cpit != classParams.end()) {
		auto& cp = cpit->second;
		if (cp.hasValue) {
			i.isIncomplete(false);
			i.type(cp.parameter.valueType());
			i.constexprValue(cp.value->constexprValue());
			i.isConstexpr(cp.value->isConstexpr());
		}
		cp.used = true;
		return;
	}

	if (!classParams.count(i.name()))
		diags.print(undeclaredIdentifier(Identifier(i.sloc(), i.name())));
}

// nothing to do for typed literals
// only isConstexpr is relevant, and the ast knows that already
void SemanticVisitor::visit(TypedLiteral<bool>& l) {}
void SemanticVisitor::visit(TypedLiteral<uint8_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<uint16_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<uint32_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<uint64_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<int8_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<int16_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<int32_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<int64_t>& l) {}
void SemanticVisitor::visit(TypedLiteral<float>& l) {}

void SemanticVisitor::visit(CastExpr& c)
{
	c.expr().accept(*this);
	c.isIncomplete(c.expr().isIncomplete());
	c.isConstexpr(c.expr().isConstexpr() && isConstexprType(c.type()));
	c.constexprValue(c.expr().constexprValue());

	switch (c.type()) {
	case Type::Bool: c.constexprValue(c.constexprValue() != 0); break;
	case Type::UInt8: c.constexprValue(c.constexprValue() & std::numeric_limits<uint8_t>::max()); break;
	case Type::UInt16: c.constexprValue(c.constexprValue() & std::numeric_limits<uint16_t>::max()); break;
	case Type::UInt32: c.constexprValue(c.constexprValue() & std::numeric_limits<uint32_t>::max()); break;
	case Type::UInt64: c.constexprValue(c.constexprValue() & std::numeric_limits<uint64_t>::max()); break;
	default:
		break;
	}

	if (!fitsIntoConstexprType(c)) {
		diags.print(constexprInvokesUB(c));
		c.isIncomplete(true);
	}
}

void SemanticVisitor::visit(UnaryExpr& u)
{
	u.expr().accept(*this);
	u.isConstexpr(u.expr().isConstexpr());
	u.isIncomplete(u.expr().isIncomplete());

	if (u.isIncomplete())
		return;

	if (u.expr().type() == Type::Float && u.op() == UnaryOperator::Negate)
		diags.print(invalidUnaryOp(u.sloc(), u.expr().type()));
	else if (u.expr().type() != Type::Bool && u.op() == UnaryOperator::Not)
		diags.print(invalidUnaryOp(u.sloc(), u.expr().type()));

	if (u.op() == UnaryOperator::Not)
		u.type(Type::Bool);
	else
		u.type(promote(u.expr().type()));

	if (!u.isConstexpr())
		return;

	switch (u.op()) {
	case UnaryOperator::Plus:
		break;

	case UnaryOperator::Minus:
		u.constexprValue(-u.expr().constexprValue());
		break;

	case UnaryOperator::Not:
		u.constexprValue(u.expr().constexprValue() != 0);
		break;

	case UnaryOperator::Negate:
		u.constexprValue(~u.expr().constexprValue());
		break;
	}

	if (!fitsIntoConstexprType(u)) {
		diags.print(constexprDoesNotFit(u));
		u.isIncomplete(true);
	}
}

void SemanticVisitor::visit(BinaryExpr& b)
{
	b.left().accept(*this);
	b.right().accept(*this);

	b.isConstexpr(b.left().isConstexpr() && b.right().isConstexpr());
	b.isIncomplete(b.left().isIncomplete() || b.right().isIncomplete());

	if (b.isIncomplete())
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
		if (b.left().type() == Type::Float || b.right().type() == Type::Float)
			diags.print(invalidBinaryOp(b.sloc(), b.left().type(), b.right().type()));
		break;

	case BinaryOperator::ShiftLeft:
	case BinaryOperator::ShiftRight:
		b.type(promote(b.left().type()));
		if (b.left().type() == Type::Float || b.right().type() == Type::Float)
			diags.print(invalidBinaryOp(b.sloc(), b.left().type(), b.right().type()));
		else if (b.right().isConstexpr() && b.right().constexprValue() >= widthOf(b.left().type()))
			diags.print(invalidShiftAmount(b));
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

	if (!b.isConstexpr())
		return;

	switch (b.op()) {
	case BinaryOperator::Plus: b.constexprValue(b.left().constexprValue() + b.right().constexprValue()); break;
	case BinaryOperator::Minus: b.constexprValue(b.left().constexprValue() - b.right().constexprValue()); break;
	case BinaryOperator::Multiply: b.constexprValue(b.left().constexprValue() * b.right().constexprValue()); break;

	case BinaryOperator::Divide:
		if (b.right().constexprValue() == 0) {
			b.isIncomplete(true);
			diags.print(constexprInvokesUB(b));
		} else {
			b.constexprValue(truncate1(b.left().constexprValue(), b.right().constexprValue()));
		}
		break;

	case BinaryOperator::Modulo:
		if (b.right().constexprValue() == 0) {
			b.isIncomplete(true);
			diags.print(constexprInvokesUB(b));
		} else {
			b.constexprValue(rem(b.left().constexprValue(), b.right().constexprValue()));
		}
		break;

	case BinaryOperator::And: b.constexprValue(b.left().constexprValue() & b.right().constexprValue()); break;
	case BinaryOperator::Or: b.constexprValue(b.left().constexprValue() | b.right().constexprValue()); break;
	case BinaryOperator::Xor: b.constexprValue(b.left().constexprValue() ^ b.right().constexprValue()); break;
	case BinaryOperator::ShiftLeft: b.constexprValue(b.left().constexprValue() << b.right().constexprValue()); break;
	case BinaryOperator::ShiftRight: b.constexprValue(b.left().constexprValue() >> b.right().constexprValue()); break;
	case BinaryOperator::BoolAnd: b.constexprValue(b.left().constexprValue() != 0 && b.right().constexprValue() != 0); break;
	case BinaryOperator::BoolOr: b.constexprValue(b.left().constexprValue() != 0 || b.right().constexprValue() != 0); break;
	case BinaryOperator::Equals: b.constexprValue(b.left().constexprValue() == b.right().constexprValue()); break;
	case BinaryOperator::NotEquals: b.constexprValue(b.left().constexprValue() != b.right().constexprValue()); break;
	case BinaryOperator::LessThan: b.constexprValue(b.left().constexprValue() < b.right().constexprValue()); break;
	case BinaryOperator::LessOrEqual: b.constexprValue(b.left().constexprValue() <= b.right().constexprValue()); break;
	case BinaryOperator::GreaterThan: b.constexprValue(b.left().constexprValue() > b.right().constexprValue()); break;
	case BinaryOperator::GreaterOrEqual: b.constexprValue(b.left().constexprValue() >= b.right().constexprValue()); break;
	}

	if (!fitsIntoConstexprType(b)) {
		diags.print(constexprDoesNotFit(b));
		b.isIncomplete(true);
	}
}

void SemanticVisitor::visit(ConditionalExpr& c)
{
	c.condition().accept(*this);
	c.ifTrue().accept(*this);
	c.ifFalse().accept(*this);

	c.isConstexpr(c.condition().isConstexpr() && c.ifTrue().isConstexpr() && c.ifFalse().isConstexpr());
	c.isIncomplete(c.condition().isIncomplete() || c.ifTrue().isIncomplete() || c.ifFalse().isIncomplete());

	if (!c.isIncomplete())
		c.type(commonType(c.ifTrue().type(), c.ifFalse().type()));

	if (c.isConstexpr())
		c.constexprValue(c.condition().constexprValue() != 0 ? c.ifTrue().constexprValue() : c.ifFalse().constexprValue());
}

Endpoint* SemanticVisitor::checkEndpointExpr(EndpointExpr& e)
{
	if (e.endpoint())
		return e.endpoint();

	Device* dev = nullptr;
	Endpoint* ep = nullptr;
	bool exprIsFullyDefined = true;

	{
		auto cpit = classParams.find(e.deviceId().name());
		auto it = globalNames.find(e.deviceId().name());

		if (isFunctionName(e.deviceId().name())) {
			diags.print(identifierIsNoDevice(e.deviceId()));
		} else if (auto se = scopes.resolve(e.deviceId().name())) {
			diags.print(
				identifierIsNoDevice(e.deviceId()),
				declaredHere(se->declaration->sloc()));
		} else if (cpit != classParams.end()) {
			exprIsFullyDefined &= cpit->second.hasValue;
			cpit->second.used = true;
			e.deviceIdIsIncomplete(!dev);
		} else if (it != globalNames.end()) {
			dev = dynamic_cast<Device*>(it->second);
			if (!dev)
				diags.print(
					identifierIsNoDevice(e.deviceId()),
					declaredHere(it->second->sloc()));
		} else {
			diags.print(undeclaredIdentifier(e.deviceId()));
		}
		e.device(dev);
	}

	{
		auto cpit = classParams.find(e.endpointId().name());
		auto it = globalNames.find(e.endpointId().name());

		if (isFunctionName(e.endpointId().name())) {
			diags.print(identifierIsNoEndpoint(e.endpointId()));
		} else if (auto se = scopes.resolve(e.endpointId().name())) {
			diags.print(
				identifierIsNoEndpoint(e.endpointId()),
				declaredHere(se->declaration->sloc()));
		} else if (cpit != classParams.end()) {
			exprIsFullyDefined &= cpit->second.hasValue;
			cpit->second.used = true;
			e.deviceIdIsIncomplete(!ep);
		} else if (it != globalNames.end()) {
			ep = dynamic_cast<Endpoint*>(it->second);
			if (!ep)
				diags.print(
					identifierIsNoEndpoint(e.endpointId()),
					declaredHere(it->second->sloc()));
		} else {
			diags.print(undeclaredIdentifier(e.endpointId()));
		}
		e.endpoint(ep);
	}

	if (!dev || !ep) {
		e.isIncomplete(true);
		return nullptr;
	}

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

	if (!isFunctionName(c.name().name())) {
		diags.print(undeclaredIdentifier(c.name()));
		c.isIncomplete(true);
		return;
	}

	c.type(Type::UInt32);

	if (c.arguments().size() != 1)
		diags.print(invalidCallArgCount(c, 1));

	if (c.arguments().size() < 1)
		return;

	if (!isIntType(c.arguments()[0]->type()))
		diags.print(invalidArgType(c, Type::UInt64, 0));

	c.isIncomplete(c.arguments()[0]->isIncomplete());
}

// nothing to do for these
void SemanticVisitor::visit(PacketEIDExpr&) {}
void SemanticVisitor::visit(PacketValueExpr&) {}
void SemanticVisitor::visit(SysTimeExpr&) {}
void SemanticVisitor::visit(TimeoutExpr&) {}

void SemanticVisitor::visit(AssignStmt& a)
{
	ScopeEntry* se = scopes.resolve(a.target().name());

	a.value().accept(*this);

	if (isFunctionName(a.target().name())) {
		diags.print(identifierIsFunction(a.sloc(), a.target().name()));
		return;
	}
	if (!se) {
		diags.print(undeclaredIdentifier(a.target()));
		return;
	}

	a.targetDecl(se->declaration);

	if (!a.value().isIncomplete() && !isContextuallyConvertibleTo(a.value(), se->declaration->type()))
		diags.print(invalidImplicitConversion(a.sloc(), a.value().type(), se->declaration->type()));
}

void SemanticVisitor::visit(WriteStmt& w)
{
	w.value().accept(*this);

	if (auto ep = checkEndpointExpr(w.target())) {
		if ((ep->access() & EndpointAccess::Write) != EndpointAccess::Write)
			diags.print(endpointNotWritable(w.target(), *ep));

		if (!w.value().isIncomplete() && !isContextuallyConvertibleTo(w.value(), ep->type()))
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
	if (!s.expr().isIncomplete() && !isIntType(s.expr().type()))
		diags.print(expectedIntType(s.expr()));

	const SwitchLabel* defaultLabel = nullptr;
	std::map<cln::cl_I, const SwitchLabel*> caseLabels;
	for (auto& se : s.entries()) {
		scopes.enter();

		for (auto& l : se.labels()) {
			if (l.expr()) {
				l.expr()->accept(*this);
				if (!l.expr()->isIncomplete()) {
					if (!isIntType(l.expr()->type())) {
						diags.print(expectedIntType(*l.expr()));
						continue;
					}

					if (!l.expr()->isConstexpr()) {
						diags.print(caseLabelInvalid(l));
						continue;
					}

					auto res = caseLabels.insert({ l.expr()->constexprValue(), &l });
					if (!res.second)
						diags.print(
							caseLabelDuplicated(l, l.expr()->constexprValue()),
							previouslyDeclaredHere(res.first->second->sloc()));
				}
			} else if (!l.expr()) {
				if (defaultLabel)
					diags.print(
						caseLabelDuplicated(l),
						previouslyDeclaredHere(defaultLabel->sloc()));

				defaultLabel = &l;
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

	d.value().accept(*this);

	if (isFunctionName(d.name().name())) {
		diags.print(redeclaration(d.name().sloc(), d.name().name()));
	} else if (auto old = scopes.resolve(d.name().name())) {
		diags.print(
			redeclaration(d.name().sloc(), d.name().name()),
			previouslyDeclaredHere(old->declaration->sloc()));
	} else {
		decl = scopes.insert(d);
	}

	if (decl && !d.value().isIncomplete() && !isContextuallyConvertibleTo(d.value(), d.type()))
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
	auto it = globalNames.find(o.sourceId().name());
	auto se = scopes.resolve(o.sourceId().name());

	o.block().accept(*this);

	if (isFunctionName(o.sourceId().name())) {
		diags.print(identifierIsFunction(o.sloc(), o.sourceId().name()));
		return;
	}

	if (it == globalNames.end() && !se)
		diags.print(undeclaredIdentifier(o.sourceId()));
	else if (se || !dynamic_cast<Device*>(it->second))
		diags.print(identifierIsNoDevice(o.sourceId()));
	else
		o.source(dynamic_cast<Device*>(it->second));
}

void SemanticVisitor::visit(OnExprBlock& o)
{
	o.condition().accept(*this);

	if (!o.condition().isIncomplete() && o.condition().type() != Type::Bool)
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

	if (!m.parameters().size())
		diags.print(classWithoutParameters(m));

	for (auto& param : m.parameters()) {
		auto res = classParams.insert({ param->name(), { *param, nullptr } });

		if (!res.second)
			diags.print(
				classParamRedefined(*param),
				previouslyDeclaredHere(res.first->second.parameter.sloc()));
	}

	checkMachineBody(m);

	for (auto& param : classParams) {
		if (!param.second.used)
			diags.print(classParamUnused(param.second.parameter));
	}

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
			switch ((*param)->type()) {
			case ClassParameter::Type::Value:
				(*arg)->accept(*this);
				if (!(*arg)->isConstexpr())
					diags.print(classArgumentInvalid((*arg)->sloc(), **param));
				else if (!isContextuallyConvertibleTo(**arg, (*param)->valueType()))
					diags.print(invalidImplicitConversion((*arg)->sloc(), (*arg)->type(), (*param)->valueType()));
				else
					classParams.insert({ (*param)->name(), { **param, nullptr, (*arg).get() } });
				break;

			case ClassParameter::Type::Device:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto dev = globalNames.find(id->name());
					if (dev == globalNames.end())
						diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
					else if (auto decl = dynamic_cast<Device*>(dev->second))
						classParams.insert({ (*param)->name(), { **param, nullptr, decl } });
					else
						diags.print(identifierIsNoDevice(Identifier(id->sloc(), id->name())));
				} else {
					diags.print(classArgumentInvalid((*arg)->sloc(), **param));
				}
				break;

			case ClassParameter::Type::Endpoint:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto ep = globalNames.find(id->name());
					if (ep == globalNames.end())
						diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
					else if (auto decl = dynamic_cast<Endpoint*>(ep->second))
						classParams.insert({ (*param)->name(), { **param, nullptr, decl } });
					else
						diags.print(identifierIsNoEndpoint(Identifier(id->sloc(), id->name())));
				} else {
					diags.print(classArgumentInvalid((*arg)->sloc(), **param));
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
