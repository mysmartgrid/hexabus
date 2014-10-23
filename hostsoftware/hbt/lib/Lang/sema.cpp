#include "Lang/sema.hpp"

#include "instantiationvisitor.hpp"
#include "Util/range.hpp"

#include <boost/format.hpp>
#include <cln/integer_io.h>

using boost::format;

namespace hbt {
namespace lang {

static std::string cptDeclStr(ClassParameter::Kind k)
{
	switch (k) {
	case ClassParameter::Kind::Value: return "integer constant expression";
	case ClassParameter::Kind::Device: return "device name";
	case ClassParameter::Kind::Endpoint: return "endpoint name";
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

static Diagnostic identifierIsNoValue(const IdentifierExpr& ident)
{
	return { DiagnosticKind::Error, &ident.sloc(), str(format("identifier '%1%' does not name a value") % ident.name()) };
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
			% cptDeclStr(cp.kind())
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

static Diagnostic identifierIsNoFunction(const Identifier& id)
{
	return { DiagnosticKind::Error, &id.sloc(), str(format("'%1%' is not a function") % id.name()) };
}

static Diagnostic cannotAssignTo(const Identifier& id)
{
	return { DiagnosticKind::Error, &id.sloc(), str(format("cannot assign to '%1%'") % id.name()) };
}

static Diagnostic gotoForbiddenIn(const GotoStmt& g, const char* scope)
{
	return { DiagnosticKind::Error, &g.sloc(), str(format("invalid use of goto in %1%") % scope) };
}

static Diagnostic classWithoutStates(const MachineClass& m)
{
	return { DiagnosticKind::Error, &m.sloc(), str(format("class %1% has no states") % m.name().name()) };
}

static Diagnostic machineWithoutStates(const MachineDefinition& m)
{
	return { DiagnosticKind::Error, &m.sloc(), str(format("machine %1% has no states") % m.name().name()) };
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



class StackedScope {
private:
	Scope nested;
	Scope*& parent;

public:
	StackedScope(Scope*& parent)
		: nested(parent), parent(parent)
	{
		parent = &nested;
	}

	~StackedScope()
	{
		parent = nested.parent();
	}

	StackedScope(StackedScope&&) = delete;

	StackedScope& operator=(StackedScope&&) = delete;
};



SemanticVisitor::SemanticVisitor(DiagnosticOutput& diagOut)
	: diags(diagOut), currentScope(&globalScope), gotoExclusionScope(nullptr)
{
	builtinFunctions.emplace_back(BuiltinFunction("second", Type::Int32, { Type::Int64 }));
	builtinFunctions.emplace_back(BuiltinFunction("minute", Type::Int32, { Type::Int64 }));
	builtinFunctions.emplace_back(BuiltinFunction("hour", Type::Int32, { Type::Int64 }));
	builtinFunctions.emplace_back(BuiltinFunction("day", Type::Int32, { Type::Int64 }));
	builtinFunctions.emplace_back(BuiltinFunction("month", Type::Int32, { Type::Int64 }));
	builtinFunctions.emplace_back(BuiltinFunction("year", Type::Int32, { Type::Int64 }));
	builtinFunctions.emplace_back(BuiltinFunction("weekday", Type::Int32, { Type::Int64 }));
	builtinFunctions.emplace_back(BuiltinFunction("now", Type::Int64, {}));

	for (auto& builtin : builtinFunctions)
		globalScope.insert(builtin);
}

void SemanticVisitor::declareInCurrentScope(Declaration& decl)
{
	auto res = currentScope->insert(decl);

	if (!res.second)
		diags.print(
			redeclaration(decl.sloc(), decl.identifier()),
			previouslyDeclaredHere(res.first->sloc()));
}

void SemanticVisitor::visit(IdentifierExpr& i)
{
	auto* se = currentScope->resolve(i.name());

	i.isIncomplete(true);

	if (!se) {
		diags.print(undeclaredIdentifier(Identifier(i.sloc(), i.name())));
		return;
	}

	if (auto* decl = dynamic_cast<DeclarationStmt*>(se)) {
		i.type(decl->type());
		i.target(decl);
		i.isIncomplete(false);
		return;
	}

	if (auto* cpv = dynamic_cast<CPValue*>(se)) {
		auto& cp = classParams.at(i.name());

		cp.used = true;
		i.target(cpv);
		i.type(cpv->type());
		i.isConstexpr(true);

		if (cp.hasValue) {
			i.isIncomplete(false);
			i.constexprValue(cp.value->constexprValue());
		}
		return;
	}

	diags.print(
		identifierIsNoValue(i),
		declaredHere(se->sloc()));
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

	if (c.isIncomplete())
		return;

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

	if (c.isIncomplete())
		return;

	c.type(commonType(c.ifTrue().type(), c.ifFalse().type()));

	if (c.isConstexpr())
		c.constexprValue(c.condition().constexprValue() != 0 ? c.ifTrue().constexprValue() : c.ifFalse().constexprValue());
}

std::pair<Declaration*, Device*> SemanticVisitor::resolveDeviceInScope(const Identifier& device)
{
	auto* decl = currentScope->resolve(device.name());

	if (!decl) {
		diags.print(undeclaredIdentifier(device));
		return { nullptr, nullptr };
	}

	if (auto* dev = dynamic_cast<Device*>(decl))
		return { dev, dev };

	if (auto* cpd = dynamic_cast<CPDevice*>(decl)) {
		auto cpit = classParams.find(device.name());

		if (cpit != classParams.end()) {
			auto& cpi = cpit->second;

			cpi.used = true;
			if (cpi.hasValue)
				return { cpd, cpi.device };
		}

		return { cpd, nullptr };
	}

	diags.print(
		identifierIsNoDevice(device),
		declaredHere(decl->sloc()));

	return { nullptr, nullptr };
}

std::pair<Declaration*, Endpoint*> SemanticVisitor::resolveEndpointInScope(const Identifier& endpoint)
{
	auto* decl = currentScope->resolve(endpoint.name());

	if (!decl) {
		diags.print(undeclaredIdentifier(endpoint));
		return { nullptr, nullptr };
	}

	if (auto* ep = dynamic_cast<Endpoint*>(decl))
		return { ep, ep };

	if (auto* cpe = dynamic_cast<CPEndpoint*>(decl)) {
		auto cpit = classParams.find(endpoint.name());

		if (cpit != classParams.end()) {
			auto& cpi = cpit->second;

			cpi.used = true;
			if (cpi.hasValue)
				return { cpe, cpi.endpoint };
		}

		return { cpe, nullptr };
	}

	diags.print(
		identifierIsNoEndpoint(endpoint),
		declaredHere(decl->sloc()));

	return { nullptr, nullptr };
}

Endpoint* SemanticVisitor::checkEndpointExpr(EndpointExpr& e)
{
	if (auto* ep = dynamic_cast<Endpoint*>(e.endpoint()))
		return ep;

	auto dev = resolveDeviceInScope(e.deviceId());
	auto ep = resolveEndpointInScope(e.endpointId());

	e.device(dev.first);
	e.endpoint(ep.first);
	e.deviceIdIsIncomplete(!dev.second);
	e.endpointIdIsIncomplete(!ep.second);

	if (e.isIncomplete())
		return nullptr;

	auto& endpoints = dev.second->endpoints();
	bool hasEP = endpoints.end() != std::find_if(endpoints.begin(), endpoints.end(),
		[ep] (const Identifier& i) {
			return i.name() == ep.second->name().name();
		});
	if (!hasEP) {
		diags.print(deviceDoesNotHave(e, dev.second->name().name(), ep.second->name().name()));
		return nullptr;
	}

	e.type(ep.second->type());

	return ep.second;
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

	auto* se = currentScope->resolve(c.name().name());
	auto* fun = dynamic_cast<BuiltinFunction*>(se);

	c.isIncomplete(true);

	if (!se) {
		diags.print(undeclaredIdentifier(c.name()));
		return;
	}

	if (!fun) {
		diags.print(identifierIsNoFunction(c.name()));
		return;
	}

	c.type(fun->returnType());
	c.target(fun);

	if (c.arguments().size() != fun->argumentTypes().size())
		diags.print(invalidCallArgCount(c, fun->argumentTypes().size()));

	auto ait = c.arguments().begin(), aend = c.arguments().end();
	auto tit = fun->argumentTypes().begin(), tend = fun->argumentTypes().end();
	bool incomplete = false;

	for (; ait != aend && tit != tend; ++ait, ++tit) {
		if (!isContextuallyConvertibleTo(**ait, *tit))
			diags.print(invalidArgType(c, *tit, ait - c.arguments().begin()));
		incomplete |= (*ait)->isIncomplete();
	}

	c.isIncomplete(incomplete);
}

void SemanticVisitor::visit(AssignStmt& a)
{
	auto* se = currentScope->resolve(a.target().name());

	a.value().accept(*this);

	if (!se) {
		diags.print(undeclaredIdentifier(a.target()));
		return;
	}

	if (auto* decl = dynamic_cast<DeclarationStmt*>(se)) {
		a.targetDecl(decl);

		if (!a.value().isIncomplete() && !isContextuallyConvertibleTo(a.value(), decl->type()))
			diags.print(invalidImplicitConversion(a.sloc(), a.value().type(), decl->type()));
		return;
	}

	diags.print(
		cannotAssignTo(a.target()),
		declaredHere(se->sloc()));
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
		StackedScope caseScope(currentScope);

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
	}
}

void SemanticVisitor::visit(BlockStmt& b)
{
	StackedScope blockScope(currentScope);

	for (auto& s : b.statements())
		s->accept(*this);
}

void SemanticVisitor::visit(DeclarationStmt& d)
{
	d.value().accept(*this);

	declareInCurrentScope(d);

	if (!d.value().isIncomplete() && !isContextuallyConvertibleTo(d.value(), d.type()))
		diags.print(invalidImplicitConversion(d.sloc(), d.value().type(), d.type()));
}

void SemanticVisitor::visit(GotoStmt& g)
{
	if (gotoExclusionScope)
		diags.print(gotoForbiddenIn(g, gotoExclusionScope));
	if (!knownStates.count(g.state().name()))
		diags.print(undeclaredIdentifier(g.state()));
}

void SemanticVisitor::visit(OnSimpleBlock& o)
{
	switch (o.trigger()) {
	case OnSimpleTrigger::Entry: gotoExclusionScope = "on entry block"; break;
	case OnSimpleTrigger::Exit: gotoExclusionScope = "on exit block"; break;
	default: break;
	}
	o.block().accept(*this);
	gotoExclusionScope = nullptr;
}

void SemanticVisitor::visit(OnPacketBlock& o)
{
	auto* se = currentScope->resolve(o.sourceId().name());

	Declaration* sourceDevice = dynamic_cast<Device*>(se);

	if (!sourceDevice) {
		auto* cpd = dynamic_cast<CPDevice*>(se);

		if (cpd)
			classParams.at(cpd->identifier()).used = true;

		sourceDevice = cpd;
	}

	if (!se) {
		diags.print(undeclaredIdentifier(o.sourceId()));
		return;
	}

	if (!sourceDevice) {
		diags.print(
			identifierIsNoDevice(o.sourceId()),
			declaredHere(se->sloc()));
	}

	o.block().accept(*this);
	o.source(sourceDevice);
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
	declareInCurrentScope(e);

	auto res = endpointsByEID.insert({ e.eid(), &e });

	if (!res.second)
		diags.print(
			eidRedefined(e),
			previouslyDeclaredHere(res.first->second->sloc()));
}

void SemanticVisitor::visit(Device& d)
{
	declareInCurrentScope(d);

	for (auto& ep : d.endpoints()) {
		auto* se = currentScope->resolve(ep.name());

		if (!se) {
			diags.print(undeclaredIdentifier(ep));
			continue;
		}

		if (!dynamic_cast<Endpoint*>(se)) {
			diags.print(
				identifierIsNoEndpoint(Identifier(ep.sloc(), ep.name())),
				declaredHere(se->sloc()));
		}
	}
}

void SemanticVisitor::checkState(State& s)
{
	StackedScope stateScope(currentScope);

	for (auto& var : s.variables())
		var.accept(*this);
	for (auto& on : s.onBlocks())
		on->accept(*this);
	if (s.always())
		s.always()->accept(*this);
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

	StackedScope machineScope(currentScope);

	for (auto& var : m.variables())
		var->accept(*this);
	for (auto& state : m.states())
		checkState(state);
}

void SemanticVisitor::visit(MachineClass& m)
{
	declareInCurrentScope(m);

	if (!m.parameters().size())
		diags.print(classWithoutParameters(m));
	if (!m.states().size())
		diags.print(classWithoutStates(m));

	StackedScope classScope(currentScope);

	for (auto& param : m.parameters()) {
		auto res = currentScope->insert(*param);
		classParams.emplace(param->identifier(), *param);

		if (!res.second)
			diags.print(
				classParamRedefined(*param),
				previouslyDeclaredHere(res.first->sloc()));
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
	declareInCurrentScope(m);

	if (!m.states().size())
		diags.print(machineWithoutStates(m));

	checkMachineBody(m);
}

void SemanticVisitor::visit(MachineInstantiation& m)
{
	declareInCurrentScope(m);

	auto* se = currentScope->resolve(m.instanceOf().name());

	if (!se) {
		diags.print(undeclaredIdentifier(m.instanceOf()));
		return;
	}

	auto mclass = dynamic_cast<MachineClass*>(se);
	if (!mclass) {
		diags.print(
			identifierIsNoClass(m.instanceOf()),
			declaredHere(se->sloc()));
		return;
	}

	m.baseClass(mclass);

	StackedScope machineScope(currentScope);

	if (m.arguments().size() != mclass->parameters().size()) {
		diags.print(invalidClassArgCount(m, mclass->parameters().size()));
	} else {
		auto arg = m.arguments().begin(), aend = m.arguments().end();
		auto param = mclass->parameters().begin(), pend = mclass->parameters().end();

		auto previousErrorCount = diags.errorCount();

		for (; arg != aend; ++arg, ++param) {
			switch ((*param)->kind()) {
			case ClassParameter::Kind::Value: {
				auto& cpv = static_cast<CPValue&>(**param);
				(*arg)->accept(*this);
				if (!(*arg)->isConstexpr())
					diags.print(classArgumentInvalid((*arg)->sloc(), cpv));
				else if (!isContextuallyConvertibleTo(**arg, cpv.type()))
					diags.print(invalidImplicitConversion((*arg)->sloc(), (*arg)->type(), cpv.type()));
				else
					classParams.insert({ cpv.name(), { cpv, (*arg).get() } });
				break;
			}

			case ClassParameter::Kind::Device:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto se = currentScope->resolve(id->name());
					auto dev = dynamic_cast<Device*>(se);

					if (!se)
						diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
					else if (!dev)
						diags.print(identifierIsNoDevice(Identifier(id->sloc(), id->name())));
					else
						classParams.insert({ (*param)->name(), { **param, dev } });
				} else {
					diags.print(classArgumentInvalid((*arg)->sloc(), **param));
				}
				break;

			case ClassParameter::Kind::Endpoint:
				if (auto id = dynamic_cast<IdentifierExpr*>(arg->get())) {
					auto se = currentScope->resolve(id->name());
					auto ep = dynamic_cast<Endpoint*>(se);

					if (!se)
						diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
					else if (!ep)
						diags.print(identifierIsNoEndpoint(Identifier(id->sloc(), id->name())));
					else
						classParams.insert({ (*param)->name(), { **param, ep } });
				} else {
					diags.print(classArgumentInvalid((*arg)->sloc(), **param));
				}
				break;

			default:
				throw "invalid class parameter type";
			}
		}

		auto hint = diags.useHint(m.sloc(), "instantiated from here");

		if (diags.errorCount() == previousErrorCount && classParams.size() == mclass->parameters().size()) {
			for (auto& param : mclass->parameters())
				currentScope->insert(dynamic_cast<Declaration&>(*param));

			InstantiationVisitor iv(*currentScope);
			m.accept(iv);
			checkMachineBody(*m.instantiation());
		}
		classParams.clear();
	}
}

void SemanticVisitor::visit(IncludeLine& i)
{
	// nothing to do
}

void SemanticVisitor::visit(TranslationUnit& t)
{
	endpointsByEID.clear();

	for (auto& item : t.items())
		item->accept(*this);
}

}
}
