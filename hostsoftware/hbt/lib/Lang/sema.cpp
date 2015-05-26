#include "hbt/Lang/sema.hpp"

#include "instantiationvisitor.hpp"
#include "hbt/Util/fatal.hpp"
#include "hbt/Util/range.hpp"

#include <boost/format.hpp>

using boost::format;

namespace hbt {
namespace lang {

static std::string cptDeclStr(ClassParameter::Kind k)
{
	switch (k) {
	case ClassParameter::Kind::Value: return "integer constant expression";
	case ClassParameter::Kind::Device: return "device name";
	case ClassParameter::Kind::DeviceList: return "device name list";
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

static Diagnostic identifierIsNoBehaviour(const Identifier& ident)
{
	return { DiagnosticKind::Error, &ident.sloc(), str(format("identifier '%1%' does not name a behaviour") % ident.name()) };
}

static Diagnostic identifierIsNoMachineClass(const Identifier& ident)
{
	return { DiagnosticKind::Error, &ident.sloc(), str(format("identifier '%1%' does not name a machine class") % ident.name()) };
}

static Diagnostic identifierIsNoBehaviourClass(const Identifier& ident)
{
	return {
		DiagnosticKind::Error,
		&ident.sloc(),
		str(format("identifier '%1%' does not name a behaviour class") % ident.name())
	};
}

static std::string endpointPath(EndpointExpr& e, bool full)
{
	std::string path;
	for (auto& elem : e.path()) {
		if (!full) {
			full = true;
			continue;
		}

		if (path.size())
			path += '.';
		path += elem.name();
	}
	return path;
}

static std::string endpointName(EndpointExpr& e, const Identifier* dev)
{
	std::string name;
	if (dev)
		name = dev->name() + '.';
	if (e.behaviour())
		name += e.behaviour()->identifier() + '.';
	name += e.endpoint()->identifier();
	return name;
}

static Diagnostic endpointPropertyMismatch(EndpointExpr& e, const std::string& prop, const Identifier* dev = nullptr)
{
	std::string path = endpointPath(e, dev != nullptr);
	std::string name = endpointName(e, dev);
	std::string aka = path == name ? "" : " (aka " + name + ")";

	return { DiagnosticKind::Error, &e.sloc(), str(format("endpoint %1%%2% %3%") % path % aka % prop) };
}

static Diagnostic endpointNotReadable(EndpointExpr& e)
{
	return endpointPropertyMismatch(e, "cannot be read");
}

static Diagnostic endpointNotWritable(EndpointExpr& e)
{
	return endpointPropertyMismatch(e, "cannot be written");
}

static Diagnostic endpointNotLive(EndpointExpr& e)
{
	return { DiagnosticKind::Error, &e.sloc(), str(format("endpoint %1% is not live") % endpointPath(e, &e.path()[0])) };
}

static Diagnostic memberReferenceAmbiguous(EndpointExpr& e, Device& dev)
{
	return endpointPropertyMismatch(e, "is ambiguous", &dev.name());
}

static Diagnostic deviceDoesNotHave(EndpointExpr& e, Device& dev)
{
	auto epPath = endpointPath(e, false);
	auto epName = endpointName(e, nullptr);
	auto epPart = epPath == epName
		? epPath
		: str(format("%1% (aka %2%)") % epPath % epName);

	auto& devPart = dev.name().name() == e.path().front().name()
		? e.path().front().name()
		: str(format("%1% (aka %2%)") % e.path().front().name() % dev.name().name());

	return { DiagnosticKind::Error, &e.sloc(), str(format("device %1% does not implement endpoint %2%") % devPart % epPart) };
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

static Diagnostic caseLabelDuplicated(const SwitchLabel& l, const util::Bigint& value)
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

static Diagnostic invalidClassArgCount(const SourceLocation& sloc, const Identifier& name, unsigned got, unsigned expected)
{
	return {
		DiagnosticKind::Error,
		&sloc,
		str(format("too %1% arguments in instantiation of class %2% (expected %3%, got %4%)")
			% (got < expected ? "few" : "many")
			% name.name()
			% expected
			% got)
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

static Diagnostic deviceListEmpty(const SourceLocation& sloc)
{
	return {
		DiagnosticKind::Error,
		&sloc,
		"device name list may not be empty"
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
	return { DiagnosticKind::Error, &m.sloc(), str(format("machine class %1% has no parameters") % m.name().name()) };
}

static Diagnostic constexprDoesNotFit(const Expr& e)
{
	return {
		DiagnosticKind::Error,
		&e.sloc(),
		str(format("value %1% overflows type %2%") % e.constexprValue() % typeName(e.type()))
	};
}

static Diagnostic expectedConstexprType(const Expr& e)
{
	return {
		DiagnosticKind::Error,
		&e.sloc(),
		str(format("expected expression of integral or boolean type, got %1%") % typeName(e.type()))
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

static Diagnostic onExitNotAllowed(const OnSimpleBlock& o)
{
	return { DiagnosticKind::Error, &o.sloc(), "on exit block not allowed here" };
}

static Diagnostic wordIsReservedHere(const SourceLocation& sloc, const std::string& name)
{
	return { DiagnosticKind::Error, &sloc, str(format("'%1%' is a reserved word in this context") % name) };
}

static Diagnostic notAllowedHere(const Identifier& id)
{
	return { DiagnosticKind::Error, &id.sloc(), str(format("'%1%' may not be used here") % id.name()) };
}

static Diagnostic entryBlockNotAllowedHere(const OnBlock& block)
{
	return { DiagnosticKind::Error, &block.sloc(), "on entry must be the first block of the state" };
}

static Diagnostic multipleExitBlocksNotAllowed(const OnBlock& block)
{
	return { DiagnosticKind::Error, &block.sloc(), "multiple exit blocks are not allowed" };
}

static Diagnostic multipleAlwaysBlocksNotAllowed(const OnBlock& block)
{
	return { DiagnosticKind::Error, &block.sloc(), "multiple always blocks are not allowed" };
}

static Diagnostic periodicBlockNotAllowedHere(const OnBlock& block)
{
	return { DiagnosticKind::Error, &block.sloc(), "periodic block not allowed after expression, always or exit blocks" };
}

static Diagnostic updateBlockNotAllowedHere(const OnBlock& block)
{
	return { DiagnosticKind::Error, &block.sloc(), "update block not allowed after expression, always or exit blocks" };
}

static Diagnostic alwaysBlockNotAllowedHere(const OnBlock& block)
{
	return { DiagnosticKind::Error, &block.sloc(), "always block not allowed after exit block" };
}

static Diagnostic exprBlockNotAllowedHere(const OnBlock& block)
{
	return { DiagnosticKind::Error, &block.sloc(), "on (expr) block not allowed after always or exit blocks" };
}



static bool fitsIntoConstexprType(Expr& e, Type t)
{
	switch (t) {
	case Type::Bool: return e.constexprValue().fitsInto<bool>(); break;
	case Type::UInt8: return e.constexprValue().fitsInto<uint8_t>(); break;
	case Type::UInt16: return e.constexprValue().fitsInto<uint16_t>(); break;
	case Type::UInt32: return e.constexprValue().fitsInto<uint32_t>(); break;
	case Type::UInt64: return e.constexprValue().fitsInto<uint64_t>(); break;
	case Type::Int8: return e.constexprValue().fitsInto<int8_t>(); break;
	case Type::Int16: return e.constexprValue().fitsInto<int16_t>(); break;
	case Type::Int32: return e.constexprValue().fitsInto<int32_t>(); break;
	case Type::Int64: return e.constexprValue().fitsInto<int64_t>(); break;
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
	: diags(diagOut), currentScope(&globalScope), liveEndpoint(nullptr), gotoExclusionScope(nullptr), behaviourDevice(nullptr),
	  isResolvingType(false), inBehaviour(false)
{
	globalScope.insert(*BuiltinFunction::second());
	globalScope.insert(*BuiltinFunction::minute());
	globalScope.insert(*BuiltinFunction::hour());
	globalScope.insert(*BuiltinFunction::day());
	globalScope.insert(*BuiltinFunction::month());
	globalScope.insert(*BuiltinFunction::year());
	globalScope.insert(*BuiltinFunction::weekday());
	globalScope.insert(*BuiltinFunction::now());
}

bool SemanticVisitor::resolveType(Expr& e)
{
	if (auto* epp = dynamic_cast<EndpointExpr*>(&e)) {
		e.isIncomplete(true);

		if (!resolveEndpointExpr(*epp, false))
			return false;

		e.isIncomplete(false);
		return true;
	}

	isResolvingType = true;
	e.accept(*this);
	isResolvingType = false;

	return !e.isIncomplete();
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
	i.target(nullptr);
	i.value(nullptr);

	if (!se) {
		diags.print(undeclaredIdentifier(Identifier(i.sloc(), i.name())));
		return;
	}

	if (auto* v = dynamic_cast<SyntheticEndpoint*>(se)) {
		i.type(v->type());
		i.target(v);
		i.isIncomplete(false);
		return;
	}

	if (auto* decl = dynamic_cast<DeclarationStmt*>(se)) {
		i.type(decl->type());
		i.target(decl);
		i.isIncomplete(false);
		return;
	}

	if (auto* cpv = dynamic_cast<CPValue*>(se)) {
		cpv->used(true);
		i.type(cpv->type());
		i.isConstexpr(true);

		auto it = classParams.find(i.name());
		if (it != classParams.end()) {
			auto& expr = dynamic_cast<CAExpr&>(*it->second).expr();
			i.isIncomplete(false);
			i.constexprValue(expr.constexprValue());
			i.value(&expr);
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

	if (c.typeSource() && resolveType(*c.typeSource()))
		c.type(c.typeSource()->type());

	c.isIncomplete((c.typeSource() && c.typeSource()->isIncomplete()) || c.expr().isIncomplete());
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
		u.constexprValue(!u.expr().constexprValue());
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
			b.constexprValue(b.left().constexprValue() / b.right().constexprValue());
		}
		break;

	case BinaryOperator::Modulo:
		if (b.right().constexprValue() == 0) {
			b.isIncomplete(true);
			diags.print(constexprInvokesUB(b));
		} else {
			b.constexprValue(b.left().constexprValue() % b.right().constexprValue());
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

bool SemanticVisitor::resolveDevices(EndpointExpr& e)
{
	auto complete = [&] (Declaration* decl, Device* dev) {
		e.devices({dev ? dev : decl});
		e.deviceIdIsIncomplete(!dev);
		return dev != nullptr;
	};

	auto* decl = currentScope->resolve(e.path().at(0).name());

	if (inBehaviour) {
		if (e.path()[0].name() != "this") {
			diags.print(notAllowedHere(e.path()[0]));
			return complete(nullptr, nullptr);
		}
		return complete(behaviourDevice, behaviourDevice);
	}

	if (!decl) {
		diags.print(undeclaredIdentifier(e.path()[0]));
		return complete(nullptr, nullptr);
	}

	if (auto* dev = dynamic_cast<Device*>(decl))
		return complete(dev, dev);

	if (auto* cpd = dynamic_cast<CPDevice*>(decl)) {
		cpd->used(true);

		auto it = classParams.find(e.path()[0].name());
		if (it != classParams.end())
			return complete(cpd, &dynamic_cast<CADevice&>(*it->second).device());

		return complete(cpd, nullptr);
	}

	if (auto* cpdl = dynamic_cast<CPDeviceList*>(decl)) {
		cpdl->used(true);

		auto it = classParams.find(e.path()[0].name());
		if (it == classParams.end())
			return complete(cpdl, nullptr);

		std::vector<Declaration*> devs;
		for (auto* dev : dynamic_cast<CADeviceList&>(*it->second).devices())
			devs.push_back(dev);

		e.devices(std::move(devs));
		e.deviceIdIsIncomplete(false);
		return true;
	}

	diags.print(
		identifierIsNoDevice(e.path()[0]),
		declaredHere(decl->sloc()));

	return complete(nullptr, nullptr);
}

Declaration* SemanticVisitor::resolveEndpointExpr(EndpointExpr& e, bool incompletePath)
{
	auto complete = [&] (Declaration* decl, Declaration* b, EndpointDeclaration* ep) -> EndpointDeclaration* {
		e.behaviour(b);
		e.endpoint(ep ? ep : decl);
		e.endpointIdIsIncomplete(!ep);

		if (!incompletePath && ep) {
			for (auto* devdec : e.devices()) {
				if (!devdec)
					continue;

				auto* dev = dynamic_cast<Device*>(devdec);
				if (!dev)
					continue;

				auto* bh = dynamic_cast<BehaviourDefinition*>(b);
				if (auto* bc = dynamic_cast<BehaviourClass*>(b)) {
					if (dev->behaviourClasses().count(bc) == 1) {
						bh = dev->behaviourClasses().find(bc)->second;
					} else {
						diags.print(memberReferenceAmbiguous(e, *dev));
						return nullptr;
					}
				}
				if (bh) {
					if (!dev->behaviours().count(bh)) {
						diags.print(deviceDoesNotHave(e, *dev));
						return nullptr;
					}
					b = bh;
					auto epit = std::find_if(bh->endpoints().begin(), bh->endpoints().end(),
						[&] (const std::unique_ptr<SyntheticEndpoint>& sep) {
							return sep->identifier() == ep->identifier();
						});
					ep = epit->get();
				} else if (!dev->endpoints().count(ep->identifier())) {
					diags.print(deviceDoesNotHave(e, *dev));
					return nullptr;
				}
			}
		}

		e.behaviour(b);
		e.endpoint(ep ? ep : decl);
		e.endpointIdIsIncomplete(!ep);
		if (ep)
			e.type(ep->type());

		return ep;
	};

	bool isLongForm = false;
	unsigned longOffset = 0;

	if (incompletePath) {
		isLongForm = e.path().size() == 2;
		longOffset = 0;
	} else {
		isLongForm = e.path().size() == 3;
		longOffset = 1;
	}

	if (isLongForm) {
		auto* bdecl = currentScope->resolve(e.path()[longOffset + 0].name());
		if (!bdecl) {
			diags.print(undeclaredIdentifier(e.path()[longOffset + 0]));
			return complete(nullptr, nullptr, nullptr);
		}

		Behaviour* b = dynamic_cast<BehaviourDefinition*>(bdecl);
		if (!b) {
			if (auto* bi = dynamic_cast<BehaviourInstantiation*>(bdecl)) {
				b = bi->instantiation();
				bdecl = bi->instantiation();
			}
		}
		if (!b)
			b = dynamic_cast<BehaviourClass*>(bdecl);
		if (!b) {
			diags.print(
				identifierIsNoBehaviour(e.path()[longOffset + 0]),
				declaredHere(bdecl->sloc()));
			return complete(nullptr, nullptr, nullptr);
		}

		auto sepit = std::find_if(b->endpoints().begin(), b->endpoints().end(),
			[&] (const std::unique_ptr<SyntheticEndpoint>& sep) {
				return sep->name().name() == e.path()[longOffset + 1].name();
			});
		if (sepit == b->endpoints().end()) {
			diags.print(undeclaredIdentifier(e.path()[longOffset + 1]));
			return complete(nullptr, nullptr, nullptr);
		}

		return complete(sepit->get(), bdecl, sepit->get());
	}

	auto* decl = currentScope->resolve(e.path()[longOffset + 0].name());

	if (!decl) {
		diags.print(undeclaredIdentifier(e.path()[longOffset + 0]));
		return complete(nullptr, nullptr, nullptr);
	}

	if (auto* cpe = dynamic_cast<CPEndpoint*>(decl)) {
		cpe->used(true);

		auto it = classParams.find(e.path()[longOffset + 0].name());
		if (it != classParams.end()) {
			if (auto* e = dynamic_cast<CAEndpoint*>(it->second)) {
				return complete(cpe, nullptr, &e->endpoint());
			} else {
				auto& se = dynamic_cast<CASyntheticEndpoint&>(*it->second);
				return complete(cpe, &se.behaviour(), &se.endpoint());
			}
		}

		return complete(cpe, nullptr, nullptr);
	}

	if (auto* ep = dynamic_cast<Endpoint*>(decl))
		return complete(ep, nullptr, ep);

	if (decl)
		diags.print(
			identifierIsNoEndpoint(e.path()[longOffset + 0]),
			declaredHere(decl->sloc()));

	return complete(nullptr, nullptr, nullptr);
}

EndpointDeclaration* SemanticVisitor::checkEndpointExpr(EndpointExpr& e, bool forRead)
{
	auto checkAccess = [&] (EndpointDeclaration* resolved) {
		if (forRead && !isResolvingType) {
			if (auto* ep = dynamic_cast<Endpoint*>(resolved)) {
				if (!liveEndpoint || liveEndpoint->endpoint() != ep || liveEndpoint->devices() != e.devices()) {
					diags.print(endpointNotLive(e));
					return false;
				}
			} else {
				auto& sep = dynamic_cast<SyntheticEndpoint&>(*resolved);
				if (!sep.readBlock()) {
					diags.print(endpointNotReadable(e));
					return false;
				}
			}
		}
		if (!forRead && !isResolvingType) {
			if (auto* ep = dynamic_cast<Endpoint*>(e.endpoint())) {
				if ((ep->access() & EndpointAccess::Write) != EndpointAccess::Write) {
					diags.print(endpointNotWritable(e));
					return false;
				}
			} else {
				auto& sep = dynamic_cast<SyntheticEndpoint&>(*resolved);
				if (!sep.write()) {
					diags.print(endpointNotWritable(e));
					return false;
				}
			}
		}
		return true;
	};

	resolveDevices(e);
	resolveEndpointExpr(e, false);
	if (e.isIncomplete())
		return nullptr;

	auto* ep = dynamic_cast<EndpointDeclaration*>(e.endpoint());

	if (!ep || !checkAccess(ep))
		return nullptr;

	return ep;
}

void SemanticVisitor::visit(EndpointExpr& e)
{
	auto decl = checkEndpointExpr(e, true);
	if (!decl)
		return;

	if (auto* ep = dynamic_cast<Endpoint*>(decl)) {
		if ((ep->access() & EndpointAccess::Broadcast) != EndpointAccess::Broadcast)
			diags.print(endpointNotReadable(e));
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

	auto decl = checkEndpointExpr(w.target(), false);
	if (!decl)
		return;

	if (!w.value().isIncomplete() && !isContextuallyConvertibleTo(w.value(), decl->type()))
		diags.print(invalidImplicitConversion(w.sloc(), w.value().type(), decl->type()));
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
	if (!s.expr().isIncomplete() && !isConstexprType(s.expr().type()))
		diags.print(expectedConstexprType(s.expr()));

	const SwitchLabel* defaultLabel = nullptr;
	std::map<util::Bigint, const SwitchLabel*> caseLabels;
	for (auto& se : s.entries()) {
		StackedScope caseScope(currentScope);

		for (auto& l : se.labels()) {
			if (l.expr()) {
				l.expr()->accept(*this);
				if (!l.expr()->isIncomplete()) {
					if (!isConstexprType(l.expr()->type())) {
						diags.print(expectedConstexprType(*l.expr()));
						continue;
					}

					if ((l.expr()->type() == Type::Bool) != (s.expr().type() == Type::Bool)) {
						diags.print(invalidImplicitConversion(l.expr()->sloc(), l.expr()->type(), s.expr().type()));
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

	if (d.typeSource()) {
		if (!resolveType(*d.typeSource()))
			return;

		d.type(d.typeSource()->type());
	}

	if (!d.value().isIncomplete() && !isContextuallyConvertibleTo(d.value(), d.type()))
		diags.print(invalidImplicitConversion(d.sloc(), d.value().type(), d.type()));
}

void SemanticVisitor::visit(GotoStmt& g)
{
	if (gotoExclusionScope) {
		diags.print(gotoForbiddenIn(g, gotoExclusionScope));
		return;
	}
	if (!knownStates.count(g.state().name())) {
		diags.print(undeclaredIdentifier(g.state()));
		return;
	}

	g.target(knownStates[g.state().name()]);
}

void SemanticVisitor::visit(OnSimpleBlock& o)
{
	bool clearScope = false;

	if (!gotoExclusionScope) {
		clearScope = true;
		switch (o.trigger()) {
		case OnSimpleTrigger::Entry: gotoExclusionScope = "on entry block"; break;
		case OnSimpleTrigger::Exit: gotoExclusionScope = "on exit block"; break;
		default: break;
		}
	}

	o.block().accept(*this);

	if (clearScope)
		gotoExclusionScope = nullptr;
}

void SemanticVisitor::visit(OnExprBlock& o)
{
	o.condition().accept(*this);

	if (!o.condition().isIncomplete() && o.condition().type() != Type::Bool)
		diags.print(invalidImplicitConversion(o.sloc(), o.condition().type(), Type::Bool));

	o.block().accept(*this);
}

void SemanticVisitor::visit(OnUpdateBlock& o)
{
	liveEndpoint = &o.endpoint();

	o.endpoint().accept(*this);
	o.block().accept(*this);

	liveEndpoint = nullptr;
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

	for (auto& epn : d.endpointNames()) {
		auto* se = currentScope->resolve(epn.name());

		if (!se) {
			diags.print(undeclaredIdentifier(epn));
			continue;
		}

		auto* ep = dynamic_cast<Endpoint*>(se);
		if (!ep) {
			diags.print(
				identifierIsNoEndpoint(epn),
				declaredHere(se->sloc()));
			continue;
		}

		d.endpoints().insert({ ep->name().name(), ep });
	}
}

void SemanticVisitor::checkState(State& s)
{
	StackedScope stateScope(currentScope);

	for (auto& var : s.variables())
		var->accept(*this);

	enum BlockOrderState {
		BOS_INIT,
		BOS_ENTRY,
		BOS_PERIODIC_UPDATE,
		BOS_EXPR,
		BOS_ALWAYS,
		BOS_EXIT,
		BOS_DONE,
	} blockOrderState = BOS_INIT;

	auto checkBlockOrder = [&] (const std::unique_ptr<OnBlock>& block) {
		auto transitionOrDiagnose = [&] (BlockOrderState limit, const Diagnostic& diag, BlockOrderState next) {
			if (blockOrderState > limit)
				diags.print(diag);
			else
				blockOrderState = next;
		};

		if (auto* simple = dynamic_cast<OnSimpleBlock*>(block.get())) {
			switch (simple->trigger()) {
			case OnSimpleTrigger::Entry:
				transitionOrDiagnose(BOS_ENTRY, entryBlockNotAllowedHere(*block), BOS_PERIODIC_UPDATE);
				return;

			case OnSimpleTrigger::Exit:
				transitionOrDiagnose(BOS_EXIT, multipleExitBlocksNotAllowed(*block), BOS_DONE);
				return;

			case OnSimpleTrigger::Periodic:
				transitionOrDiagnose(BOS_PERIODIC_UPDATE, periodicBlockNotAllowedHere(*block), BOS_PERIODIC_UPDATE);
				return;

			case OnSimpleTrigger::Always:
				if (blockOrderState == BOS_ALWAYS)
					diags.print(multipleAlwaysBlocksNotAllowed(*block));
				else
					transitionOrDiagnose(BOS_ALWAYS, alwaysBlockNotAllowedHere(*block), BOS_ALWAYS);
				return;
			}
		}

		if (auto* expr = dynamic_cast<OnExprBlock*>(block.get())) {
			transitionOrDiagnose(BOS_EXPR, exprBlockNotAllowedHere(*block), BOS_EXPR);
			return;
		}

		if (auto* update = dynamic_cast<OnUpdateBlock*>(block.get())) {
			transitionOrDiagnose(BOS_PERIODIC_UPDATE, updateBlockNotAllowedHere(*block), BOS_PERIODIC_UPDATE);
			return;
		}

		hbt_unreachable();
	};

	for (auto& on : s.onBlocks()) {
		checkBlockOrder(on);
		on->accept(*this);
	}
}

void SemanticVisitor::checkMachineBody(MachineBody& m)
{
	knownStates.clear();

	for (auto& state : m.states()) {
		state.id(knownStates.size());

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

bool SemanticVisitor::declareClassParams(std::vector<std::unique_ptr<ClassParameter>>& params, bool forbidContextualIdentifiers)
{
	auto errC = diags.errorCount();

	for (auto& param : params) {
		if (CPValue* cpv = dynamic_cast<CPValue*>(param.get())) {
			if (cpv->typeSource()) {
				if (resolveType(*cpv->typeSource()))
					cpv->type(cpv->typeSource()->type());
			}
		}

		if (forbidContextualIdentifiers && param->name() == "this")
			diags.print(wordIsReservedHere(param->sloc(), param->name()));

		auto res = currentScope->insert(*param);
		if (!res.second)
			diags.print(
				classParamRedefined(*param),
				previouslyDeclaredHere(res.first->sloc()));
	}

	return errC == diags.errorCount();
}

void SemanticVisitor::visit(MachineClass& m)
{
	declareInCurrentScope(m);

	if (!m.parameters().size())
		diags.print(classWithoutParameters(m));
	if (!m.states().size())
		diags.print(classWithoutStates(m));

	StackedScope classScope(currentScope);

	if (declareClassParams(m.parameters(), false)) {
		checkMachineBody(m);

		for (auto& param : m.parameters()) {
			if (!param->used())
				diags.print(classParamUnused(*param));
		}
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

bool SemanticVisitor::instantiateClassParams(const SourceLocation& sloc, const Identifier& className,
		std::vector<std::unique_ptr<ClassParameter>>& params,
		std::vector<std::unique_ptr<ClassArgument>>& args)
{
	if (params.size() != args.size()) {
		diags.print(invalidClassArgCount(sloc, className, args.size(), params.size()));
		return false;
	}

	auto ait = args.begin(), aend = args.end();
	auto pit = params.begin(), pend = params.end();

	auto previousErrorCount = diags.errorCount();

	for (; ait != aend; ++ait, ++pit) {
		auto& arg = *ait;
		auto& param = **pit;

		switch (param.kind()) {
		case ClassParameter::Kind::Value: {
			auto* caexpr = dynamic_cast<CAExpr*>(arg.get());
			if (!caexpr) {
				diags.print(classArgumentInvalid(arg->sloc(), param));
				break;
			}
			auto& expr = caexpr->expr();
			auto& cpv = static_cast<CPValue&>(param);
			if (cpv.typeSource()) {
				if (!resolveType(*cpv.typeSource()))
					continue;
				cpv.type(cpv.typeSource()->type());
			}
			expr.accept(*this);
			if (!expr.isConstexpr()) {
				diags.print(classArgumentInvalid(expr.sloc(), cpv));
			} else if (!isContextuallyConvertibleTo(expr, cpv.type())) {
				diags.print(invalidImplicitConversion(expr.sloc(), expr.type(), cpv.type()));
			} else {
				classParams.emplace(cpv.name(), arg.get());
				currentScope->insert(param);
			}
			break;
		}

		case ClassParameter::Kind::Device: {
			auto* caexpr = dynamic_cast<CAExpr*>(arg.get());
			if (!caexpr) {
				diags.print(classArgumentInvalid(arg->sloc(), param));
				break;
			}
			auto& expr = caexpr->expr();
			auto* id = dynamic_cast<IdentifierExpr*>(&expr);
			if (!id) {
				diags.print(classArgumentInvalid(expr.sloc(), param));
				break;
			}
			auto* se = currentScope->resolve(id->name());
			if (!se) {
				diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
				break;
			}
			auto* dev = dynamic_cast<Device*>(se);
			if (!dev) {
				diags.print(identifierIsNoDevice(Identifier(id->sloc(), id->name())));
				break;
			}
			arg.reset(new CADevice(expr.sloc(), dev));
			classParams.emplace(param.name(), arg.get());
			currentScope->insert(param);
			break;
		}

		case ClassParameter::Kind::DeviceList: {
			auto* calist = dynamic_cast<CAIdList*>(arg.get());
			if (!calist) {
				diags.print(classArgumentInvalid(arg->sloc(), param));
				break;
			}
			auto& ids = calist->ids();
			if (ids.empty()) {
				diags.print(deviceListEmpty(calist->sloc()));
				break;
			}
			std::vector<Device*> devs;
			for (const auto& id : ids) {
				auto* se = currentScope->resolve(id.name());
				if (!se) {
					diags.print(undeclaredIdentifier(id));
					continue;
				}
				auto* dev = dynamic_cast<Device*>(se);
				if (!dev) {
					diags.print(identifierIsNoDevice(id));
					continue;
				}
				devs.push_back(dev);
			}
			arg.reset(new CADeviceList(calist->sloc(), std::move(devs)));
			classParams.emplace(param.name(), arg.get());
			currentScope->insert(param);
			break;
		}

		case ClassParameter::Kind::Endpoint: {
			auto* caexpr = dynamic_cast<CAExpr*>(arg.get());
			if (!caexpr) {
				diags.print(classArgumentInvalid(arg->sloc(), param));
				break;
			}
			auto& expr = caexpr->expr();
			if (auto* id = dynamic_cast<IdentifierExpr*>(&expr)) {
				auto* se = currentScope->resolve(id->name());
				if (!se) {
					diags.print(undeclaredIdentifier(Identifier(id->sloc(), id->name())));
					break;
				}
				auto* ep = dynamic_cast<Endpoint*>(se);
				if (!ep) {
					diags.print(identifierIsNoEndpoint(Identifier(id->sloc(), id->name())));
					break;
				}
				arg.reset(new CAEndpoint(id->sloc(), ep));
				classParams.emplace(param.name(), arg.get());
				currentScope->insert(param);
				break;
			}
			if (auto* epe = dynamic_cast<EndpointExpr*>(&expr)) {
				if (epe->path().size() != 2) {
					diags.print(classArgumentInvalid(expr.sloc(), param));
					break;
				}

				if (!resolveEndpointExpr(*epe, true))
					break;

				if (auto* ep = dynamic_cast<Endpoint*>(epe->endpoint())) {
					arg.reset(new CAEndpoint(epe->sloc(), ep));
					classParams.emplace(param.name(), arg.get());
					currentScope->insert(param);
				} else {
					auto& sep = dynamic_cast<SyntheticEndpoint&>(*epe->endpoint());
					arg.reset(new CASyntheticEndpoint(epe->sloc(), epe->behaviour(), &sep));
					classParams.emplace(param.name(), arg.get());
					currentScope->insert(param);
				}

				break;
			}
			diags.print(classArgumentInvalid(caexpr->sloc(), param));
			break;
		}

		default:
			throw "invalid class parameter type";
		}
	}

	return diags.errorCount() == previousErrorCount;
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
			identifierIsNoMachineClass(m.instanceOf()),
			declaredHere(se->sloc()));
		return;
	}

	m.baseClass(mclass);

	StackedScope machineScope(currentScope);

	if (!instantiateClassParams(m.sloc(), mclass->name(), mclass->parameters(), m.arguments()))
		return;

	auto hint = diags.useHint(m.sloc(), "instantiated from here");
	checkMachineBody(*mclass);

	InstantiationVisitor iv;
	m.accept(iv);

	classParams.clear();
}

bool SemanticVisitor::checkBehaviourBody(Behaviour& b, Device* dev)
{
	behaviourDevice = dev;
	inBehaviour = true;
	gotoExclusionScope = "behaviours";

	StackedScope behaviourScope(currentScope);

	auto errorsAtStart = diags.errorCount();

	if (dev)
		currentScope->insert("this", *dev);

	for (auto& var : b.variables())
		var->accept(*this);

	std::map<std::string, Declaration*> syntheticNames;
	for (auto& ep : b.endpoints()) {
		if (!syntheticNames.insert({ ep->name().name(), ep.get() }).second) {
			diags.print(
				redeclaration(ep->name().sloc(), ep->name().name()),
				previouslyDeclaredHere(syntheticNames[ep->name().name()]->sloc()));
		}

		if (ep->readBlock()) {
			ep->readBlock()->accept(*this);
			ep->readValue()->accept(*this);

			if (!ep->readValue()->isIncomplete() && !isAssignableFrom(ep->type(), ep->readValue()->type()))
				diags.print(invalidImplicitConversion(ep->readValue()->sloc(), ep->readValue()->type(), ep->type()));
		}

		if (ep->write()) {
			StackedScope writeScope(currentScope);

			currentScope->insert("value", *ep);
			ep->write()->accept(*this);
		}
	}

	for (auto& on : b.onBlocks()) {
		if (auto* simple = dynamic_cast<OnSimpleBlock*>(on.get())) {
			if (simple->trigger() == OnSimpleTrigger::Exit)
				diags.print(onExitNotAllowed(*simple));
		}

		on->accept(*this);
	}

	gotoExclusionScope = nullptr;
	inBehaviour = false;

	return diags.errorCount() == errorsAtStart;
}

void SemanticVisitor::visit(BehaviourClass& b)
{
	declareInCurrentScope(b);

	StackedScope classScope(currentScope);

	if (declareClassParams(b.parameters(), true)) {
		checkBehaviourBody(b, nullptr);

		for (auto& param : b.parameters()) {
			if (!param->used())
				diags.print(classParamUnused(*param));
		}
	}

	classParams.clear();
}

void SemanticVisitor::visit(BehaviourDefinition& b)
{
	declareInCurrentScope(b);

	auto* se = currentScope->resolve(b.device().name());
	if (!se) {
		diags.print(undeclaredIdentifier(b.device()));
		return;
	}
	auto* target = dynamic_cast<Device*>(se);
	if (!target) {
		diags.print(
			identifierIsNoDevice(b.device()),
			declaredHere(se->sloc()));
		return;
	}

	if (!checkBehaviourBody(b, target))
		return;

	target->behaviours().insert(&b);
}

void SemanticVisitor::visit(BehaviourInstantiation& b)
{
	declareInCurrentScope(b);

	auto* set = currentScope->resolve(b.device().name());
	auto* sec = currentScope->resolve(b.instanceOf().name());
	if (!set) {
		diags.print(undeclaredIdentifier(b.device()));
		return;
	}
	if (!sec) {
		diags.print(undeclaredIdentifier(b.instanceOf()));
		return;
	}

	auto* target = dynamic_cast<Device*>(set);
	auto* bclass = dynamic_cast<BehaviourClass*>(sec);
	if (!target) {
		diags.print(
			identifierIsNoDevice(b.device()),
			declaredHere(set->sloc()));
		return;
	}
	if (!bclass) {
		diags.print(
			identifierIsNoBehaviourClass(b.instanceOf()),
			declaredHere(sec->sloc()));
		return;
	}

	b.baseClass(bclass);

	StackedScope instScope(currentScope);

	if (!instantiateClassParams(b.sloc(), bclass->name(), bclass->parameters(), b.arguments()))
		return;

	auto hint = diags.useHint(b.sloc(), "instantiated from here");

	if (!checkBehaviourBody(*bclass, target))
		return;

	InstantiationVisitor iv;
	b.accept(iv);

	checkBehaviourBody(*b.instantiation(), target);
	target->behaviours().insert(b.instantiation());
	target->behaviourClasses().insert({ bclass, b.instantiation() });
	classParams.clear();
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
