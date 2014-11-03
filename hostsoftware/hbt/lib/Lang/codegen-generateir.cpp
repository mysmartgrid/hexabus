#include "Lang/codegen.hpp"

#include <cstring>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include <boost/format.hpp>

#include "IR/module.hpp"
#include "Lang/ast.hpp"
#include "Lang/sema-scope.hpp"

namespace hbt {
namespace lang {

namespace {

struct CodegenCantDo : public std::runtime_error {
	using runtime_error::runtime_error;
};



static ir::Type irtypeFor(Type t)
{
	switch (t) {
	case Type::Bool: return ir::Type::Bool;
	case Type::UInt8: return ir::Type::UInt8;
	case Type::UInt16: return ir::Type::UInt16;
	case Type::UInt32: return ir::Type::UInt32;
	case Type::UInt64: return ir::Type::UInt64;
	case Type::Int8: return ir::Type::Int8;
	case Type::Int16: return ir::Type::Int16;
	case Type::Int32: return ir::Type::Int32;
	case Type::Int64: return ir::Type::Int64;
	case Type::Float: return ir::Type::Float;
	}
}

static ir::ArithOp arithOpFor(BinaryOperator binop)
{
	switch (binop) {
	case BinaryOperator::Plus: return ir::ArithOp::Add;
	case BinaryOperator::Minus: return ir::ArithOp::Sub;
	case BinaryOperator::Multiply: return ir::ArithOp::Mul;
	case BinaryOperator::Divide: return ir::ArithOp::Div;
	case BinaryOperator::Modulo: return ir::ArithOp::Mod;
	case BinaryOperator::And: return ir::ArithOp::And;
	case BinaryOperator::Or: return ir::ArithOp::Or;
	case BinaryOperator::Xor: return ir::ArithOp::Xor;
	case BinaryOperator::BoolAnd: return ir::ArithOp::And;
	case BinaryOperator::BoolOr: return ir::ArithOp::Or;
	case BinaryOperator::Equals: return ir::ArithOp::Eq;
	case BinaryOperator::NotEquals: return ir::ArithOp::Neq;
	case BinaryOperator::LessThan: return ir::ArithOp::Lt;
	case BinaryOperator::LessOrEqual: return ir::ArithOp::Le;
	case BinaryOperator::GreaterThan: return ir::ArithOp::Gt;
	case BinaryOperator::GreaterOrEqual: return ir::ArithOp::Ge;
	case BinaryOperator::ShiftLeft: return ir::ArithOp::Shl;
	case BinaryOperator::ShiftRight: return ir::ArithOp::Shr;
	}
}



struct EPMemory {
	unsigned value, live;

	friend bool operator<(const EPMemory& a, const EPMemory& b)
	{
		return a.value < b.value || a.live < b.live;
	}
};

class MemoryMap {
private:
	std::map<const DeclarationStmt*, unsigned> _declarations;
	std::map<const OnExprBlock*, unsigned> _exprGuards;
	std::map<const MachineDefinition*, unsigned> _stateVars;
	unsigned _top;

public:
	MemoryMap()
		: _top(0)
	{}

	bool has(const DeclarationStmt* decl) const
	{
		return _declarations.count(decl);
	}

	unsigned addressOf(const DeclarationStmt* decl) const
	{
		return _declarations.at(decl);
	}

	void declare(const DeclarationStmt* decl)
	{
		_declarations[decl] = _top;
		_top += sizeOf(decl->type());
	}

	unsigned newAnonymous(Type type)
	{
		unsigned at = _top;
		_top += sizeOf(type);
		return at;
	}

	unsigned guardFor(const OnExprBlock* o)
	{
		if (!_exprGuards.count(o)) {
			_exprGuards.insert({ o, _top });
			_top++;
		}
		return _exprGuards[o];
	}

	bool hasGuardFor(const OnExprBlock* o)
	{
		return _exprGuards.count(o);
	}
};

class NameGenerator {
private:
	unsigned valueCount, labelCount;
	std::string prefix;

public:
	NameGenerator(std::string prefix)
		: prefix(std::move(prefix)), valueCount(0), labelCount(0)
	{}

	std::string newName()
	{
		return str(boost::format("%%%1%v%2%") % prefix % valueCount++);
	}

	std::string newLabel()
	{
		return str(boost::format("%1%L%2%") % prefix % labelCount++);
	}
};



struct CGContext {
	ir::ModuleBuilder& mb;
	MemoryMap& memMap;
	NameGenerator& ng;
	const Device* forDev;
	const ir::BasicBlock* exitChange;

	std::map<const Declaration*, const ir::Value*> valueForDecl;
	std::map<ir::BasicBlock*, const ir::Value*> nextStatesByOrigin;

	ir::BasicBlock* newBlock() { return mb.newBlock(ng.newLabel()); }
	std::string newName() { return ng.newName(); }
};

class ExprCG : public ASTVisitor {
private:
	CGContext& cgc;
	const ir::BasicBlock* guardExit;

	ir::BasicBlock* _inBlock;
	const ir::Value* _value;

protected:
	virtual void visit(IdentifierExpr&) override;
	virtual void visit(TypedLiteral<bool>&) override;
	virtual void visit(TypedLiteral<uint8_t>&) override;
	virtual void visit(TypedLiteral<uint16_t>&) override;
	virtual void visit(TypedLiteral<uint32_t>&) override;
	virtual void visit(TypedLiteral<uint64_t>&) override;
	virtual void visit(TypedLiteral<int8_t>&) override;
	virtual void visit(TypedLiteral<int16_t>&) override;
	virtual void visit(TypedLiteral<int32_t>&) override;
	virtual void visit(TypedLiteral<int64_t>&) override;
	virtual void visit(TypedLiteral<float>&) override;
	virtual void visit(CastExpr&) override;
	virtual void visit(UnaryExpr&) override;
	virtual void visit(BinaryExpr&) override;
	virtual void visit(ConditionalExpr&) override;
	virtual void visit(EndpointExpr&) override;
	virtual void visit(CallExpr&) override;

	virtual void visit(AssignStmt&) override {}
	virtual void visit(WriteStmt&) override {}
	virtual void visit(IfStmt&) override {}
	virtual void visit(SwitchStmt&) override {}
	virtual void visit(BlockStmt&) override {}
	virtual void visit(DeclarationStmt&) override {}
	virtual void visit(GotoStmt&) override {}

	virtual void visit(OnSimpleBlock&) override {}
	virtual void visit(OnExprBlock&) override {}
	virtual void visit(OnUpdateBlock&) override {}

	virtual void visit(Endpoint&) override {}
	virtual void visit(Device&) override {}
	virtual void visit(MachineClass&) override {}
	virtual void visit(MachineDefinition&) override {}
	virtual void visit(MachineInstantiation&) override {}
	virtual void visit(IncludeLine&) override {}
	virtual void visit(TranslationUnit&) override {}

	ExprCG(CGContext& cgc, const ir::BasicBlock* guardExit, ir::BasicBlock* block)
		: cgc(cgc), guardExit(guardExit), _inBlock(block)
	{}

	template<typename LitConv, typename Lit>
	void loadInt(Lit& l)
	{
		_value = _inBlock->append(ir::LoadIntInsn(cgc.newName(), irtypeFor(l.type()), LitConv(l.value())));
	}

	ExprCG runRecursive(Expr& e, Type target, ir::BasicBlock* entry = nullptr)
	{
		return run(cgc, entry ? entry : _inBlock, e, target, guardExit);
	}

public:
	ExprCG(ExprCG&&) = default;
	ExprCG& operator=(ExprCG&&) = default;

	static ExprCG run(CGContext& cgc, ir::BasicBlock* block, Expr& expr, Type target, const ir::BasicBlock* onUnavail = nullptr)
	{
		ExprCG cg(cgc, onUnavail, block);

		expr.accept(cg);
		if (cg._value->type() != irtypeFor(target))
			cg._value = cg._inBlock->append(ir::CastInsn(cgc.newName(), irtypeFor(target), cg._value));

		return cg;
	}

	ir::BasicBlock* finalBlock() { return _inBlock; }
	const ir::Value* finalValue() { return _value; }
};

void ExprCG::visit(IdentifierExpr& i)
{
	auto* decl = static_cast<const DeclarationStmt*>(i.target());

	if (cgc.memMap.has(decl))
		_value = _inBlock->append(ir::LoadInsn(cgc.newName(), irtypeFor(decl->type()), cgc.memMap.addressOf(decl)));
	else
		_value = cgc.valueForDecl.at(i.target());
}

void ExprCG::visit(TypedLiteral<bool>& l) { loadInt<uint64_t>(l); }
void ExprCG::visit(TypedLiteral<uint8_t>& l) { loadInt<uint64_t>(l); }
void ExprCG::visit(TypedLiteral<uint16_t>& l) { loadInt<uint64_t>(l); }
void ExprCG::visit(TypedLiteral<uint32_t>& l) { loadInt<uint64_t>(l); }
void ExprCG::visit(TypedLiteral<uint64_t>& l) { loadInt<uint64_t>(l); }
void ExprCG::visit(TypedLiteral<int8_t>& l) { loadInt<int64_t>(l); }
void ExprCG::visit(TypedLiteral<int16_t>& l) { loadInt<int64_t>(l); }
void ExprCG::visit(TypedLiteral<int32_t>& l) { loadInt<int64_t>(l); }
void ExprCG::visit(TypedLiteral<int64_t>& l) { loadInt<int64_t>(l); }

void ExprCG::visit(TypedLiteral<float>& l)
{
	_value = _inBlock->append(ir::LoadFloatInsn(cgc.newName(), l.value()));
}

void ExprCG::visit(CastExpr& c)
{
	auto sub = runRecursive(c.expr(), c.expr().type());
	_inBlock = sub.finalBlock();
	_value = sub.finalBlock()->append(ir::CastInsn(cgc.newName(), irtypeFor(c.type()), sub.finalValue()));
}

void ExprCG::visit(UnaryExpr& u)
{
	auto sub = runRecursive(u.expr(), promote(u.expr().type()));
	_inBlock = sub.finalBlock();

	switch (u.op()) {
	case UnaryOperator::Plus:
		break;

	case UnaryOperator::Minus: {
		auto zero = _inBlock->append(ir::LoadIntInsn(cgc.newName(), sub.finalValue()->type(), 0));
		_value = _inBlock->append(
			ir::ArithmeticInsn(
				cgc.newName(), irtypeFor(u.type()), ir::ArithOp::Sub, zero, sub.finalValue()));
		break;
	}

	case UnaryOperator::Not: {
		auto one = _inBlock->append(ir::LoadIntInsn(cgc.newName(), ir::Type::Bool, 1));
		_value = _inBlock->append(
			ir::ArithmeticInsn(
				cgc.newName(), ir::Type::Bool, ir::ArithOp::Xor, sub.finalValue(), one));
		break;
	}

	case UnaryOperator::Negate: {
		unsigned maskBits = 0;
		switch (sub.finalValue()->type()) {
		case ir::Type::Bool: maskBits = 1; break;
		case ir::Type::Int8:
		case ir::Type::UInt8: maskBits = 8; break;
		case ir::Type::Int16:
		case ir::Type::UInt16: maskBits = 16; break;
		case ir::Type::Int32:
		case ir::Type::UInt32: maskBits = 32; break;
		case ir::Type::Int64:
		case ir::Type::UInt64: maskBits = 64; break;
		default: break;
		}
		uint64_t mask = uint64_t(-1) >> (64 - maskBits);
		auto allOnes = _inBlock->append(ir::LoadIntInsn(cgc.newName(), sub.finalValue()->type(), mask));
		_value = _inBlock->append(
			ir::ArithmeticInsn(
				cgc.newName(), allOnes->type(), ir::ArithOp::Xor, sub.finalValue(), allOnes));
		break;
	}
	}
}

void ExprCG::visit(BinaryExpr& b)
{
	if (b.op() == BinaryOperator::BoolAnd || b.op() == BinaryOperator::BoolOr) {
		auto rightEntry = cgc.newBlock();
		auto finalBlock = cgc.newBlock();

		auto leftCG = runRecursive(b.left(), Type::Bool);
		auto rightCG = runRecursive(b.right(), Type::Bool, rightEntry);

		if (b.op() == BinaryOperator::BoolAnd)
			leftCG.finalBlock()->append(ir::SwitchInsn(leftCG.finalValue(), { { 1, rightEntry } }, finalBlock));
		else
			leftCG.finalBlock()->append(ir::SwitchInsn(leftCG.finalValue(), { { 0, rightEntry } }, finalBlock));

		rightCG.finalBlock()->append(ir::JumpInsn(finalBlock));

		_inBlock = finalBlock;
		_value = finalBlock->append(
			ir::PhiInsn(
				cgc.newName(), ir::Type::Bool,
				{
					{ leftCG.finalBlock(), leftCG.finalValue() },
					{ rightCG.finalBlock(), rightCG.finalValue() }
				}));

		return;
	}

	Type lType = promote(b.left().type());
	Type rType = promote(b.right().type());
	Type resType;

	switch (b.op()) {
	case BinaryOperator::Plus:
	case BinaryOperator::Minus:
	case BinaryOperator::Multiply:
	case BinaryOperator::Divide:
	case BinaryOperator::Modulo:
	case BinaryOperator::And:
	case BinaryOperator::Or:
	case BinaryOperator::Xor:
		resType = commonType(b.left().type(), b.right().type());
		lType = resType;
		rType = resType;
		break;
	case BinaryOperator::BoolAnd:
	case BinaryOperator::BoolOr:
	case BinaryOperator::Equals:
	case BinaryOperator::NotEquals:
	case BinaryOperator::LessThan:
	case BinaryOperator::LessOrEqual:
	case BinaryOperator::GreaterThan:
	case BinaryOperator::GreaterOrEqual:
		resType = Type::Bool;
		lType = commonType(lType, rType);
		rType = lType;
		break;
	case BinaryOperator::ShiftLeft:
	case BinaryOperator::ShiftRight:
		rType = Type::UInt32;
		resType = promote(b.left().type());
		break;
	}

	auto rightEntry = cgc.newBlock();
	auto leftCG = runRecursive(b.left(), lType);
	auto rightCG = runRecursive(b.right(), rType, rightEntry);

	leftCG.finalBlock()->append(ir::JumpInsn(rightEntry));
	_inBlock = rightCG.finalBlock();
	_value = _inBlock->append(
		ir::ArithmeticInsn(
			cgc.newName(), irtypeFor(resType), arithOpFor(b.op()),
			leftCG.finalValue(), rightCG.finalValue()));
}

void ExprCG::visit(ConditionalExpr& c)
{
	auto common = commonType(c.ifTrue().type(), c.ifFalse().type());
	auto leftEntry = cgc.newBlock();
	auto rightEntry = cgc.newBlock();

	auto condCG = runRecursive(c.condition(), common);
	auto leftCG = runRecursive(c.ifTrue(), common, leftEntry);
	auto rightCG = runRecursive(c.ifFalse(), common, rightEntry);

	condCG.finalBlock()->append(ir::SwitchInsn(condCG.finalValue(), { {1, leftEntry} }, rightEntry));

	_inBlock = cgc.newBlock();
	_value = _inBlock->append(
		ir::PhiInsn(
			cgc.newName(), irtypeFor(common),
			{
				{ leftCG.finalBlock(), leftCG.finalValue() },
				{ rightCG.finalBlock(), rightCG.finalValue() }
			}));
}

void ExprCG::visit(EndpointExpr& e)
{
	Endpoint* ep = static_cast<Endpoint*>(e.endpoint());

	_value = _inBlock->append(ir::LoadSpecialInsn(cgc.newName(), irtypeFor(ep->type()), ir::SpecialVal::SourceVal));
}

void ExprCG::visit(CallExpr& c)
{
	std::vector<const ir::Value*> args;
	auto* fun = static_cast<BuiltinFunction*>(c.target());

	auto ait = c.arguments().begin(), aend = c.arguments().end();
	auto tit = fun->argumentTypes().begin(), tend = fun->argumentTypes().end();
	for (; ait != aend && tit != tend; ++ait, ++tit) {
		auto sub = runRecursive(**ait, *tit);

		args.push_back(sub.finalValue());
		_inBlock = sub.finalBlock();
	}

	if (fun == BuiltinFunction::now())
		_value = _inBlock->append(ir::LoadSpecialInsn(cgc.newName(), ir::Type::Int64, ir::SpecialVal::SysTime));
	else if (fun == BuiltinFunction::second())
		_value = _inBlock->append(ir::ExtractDatePartInsn(cgc.newName(), ir::DatePart::Second, args[0]));
	else if (fun == BuiltinFunction::minute())
		_value = _inBlock->append(ir::ExtractDatePartInsn(cgc.newName(), ir::DatePart::Minute, args[0]));
	else if (fun == BuiltinFunction::hour())
		_value = _inBlock->append(ir::ExtractDatePartInsn(cgc.newName(), ir::DatePart::Hour, args[0]));
	else if (fun == BuiltinFunction::day())
		_value = _inBlock->append(ir::ExtractDatePartInsn(cgc.newName(), ir::DatePart::Day, args[0]));
	else if (fun == BuiltinFunction::month())
		_value = _inBlock->append(ir::ExtractDatePartInsn(cgc.newName(), ir::DatePart::Month, args[0]));
	else if (fun == BuiltinFunction::year())
		_value = _inBlock->append(ir::ExtractDatePartInsn(cgc.newName(), ir::DatePart::Year, args[0]));
	else if (fun == BuiltinFunction::weekday())
		_value = _inBlock->append(ir::ExtractDatePartInsn(cgc.newName(), ir::DatePart::Weekday, args[0]));
	else
		throw CodegenCantDo("unknown builtins");
}

class StmtCG : public ASTVisitor {
private:
	CGContext& cgc;

	ir::BasicBlock* _inBlock;

	const ir::Value* eval(Expr& e, Type target)
	{
		auto val = ExprCG::run(cgc, _inBlock, e, target);
		_inBlock = val.finalBlock();
		return val.finalValue();
	}

	void unifyIncomingUpdates(const std::map<const Declaration*, const ir::Value*>& preForkValues,
			const std::map<const ir::BasicBlock*, std::map<const Declaration*, const ir::Value*>>& maps)
	{
		std::map<const Declaration*, const ir::Value*> result;
		std::set<const Declaration*> updatedDecls;

		for (auto& map : maps)
			for (auto& pair : map.second)
				updatedDecls.insert(pair.first);

		for (auto* decl : updatedDecls) {
			ir::PhiInsn::sources_type sources;
			bool valueChanged = false;

			for (auto& map : maps) {
				if (map.second.count(decl)) {
					sources.insert({ map.first, map.second.at(decl) });
					valueChanged |= map.second.at(decl) != preForkValues.at(decl);
				} else {
					sources.insert({ map.first, preForkValues.at(decl) });
				}
			}

			if (!valueChanged)
				continue;

			auto type = dynamic_cast<const DeclarationStmt&>(*decl).type();
			cgc.valueForDecl[decl] = _inBlock->append(
				ir::PhiInsn(
					cgc.newName(), irtypeFor(type), std::move(sources)));
		}
	}

protected:
	StmtCG(CGContext& cgc, ir::BasicBlock* block)
		: cgc(cgc), _inBlock(block)
	{}

	virtual void visit(IdentifierExpr&) override {}
	virtual void visit(TypedLiteral<bool>&) override {}
	virtual void visit(TypedLiteral<uint8_t>&) override {}
	virtual void visit(TypedLiteral<uint16_t>&) override {}
	virtual void visit(TypedLiteral<uint32_t>&) override {}
	virtual void visit(TypedLiteral<uint64_t>&) override {}
	virtual void visit(TypedLiteral<int8_t>&) override {}
	virtual void visit(TypedLiteral<int16_t>&) override {}
	virtual void visit(TypedLiteral<int32_t>&) override {}
	virtual void visit(TypedLiteral<int64_t>&) override {}
	virtual void visit(TypedLiteral<float>&) override {}
	virtual void visit(CastExpr&) override {}
	virtual void visit(UnaryExpr&) override {}
	virtual void visit(BinaryExpr&) override {}
	virtual void visit(ConditionalExpr&) override {}
	virtual void visit(EndpointExpr&) override {}
	virtual void visit(CallExpr&) override {}

	virtual void visit(AssignStmt&) override;
	virtual void visit(WriteStmt&) override;
	virtual void visit(IfStmt&) override;
	virtual void visit(SwitchStmt&) override;
	virtual void visit(BlockStmt&) override;
	virtual void visit(DeclarationStmt&) override;
	virtual void visit(GotoStmt&) override;

	virtual void visit(OnSimpleBlock&) override {}
	virtual void visit(OnExprBlock&) override {}
	virtual void visit(OnUpdateBlock&) override {}

	virtual void visit(Endpoint&) override {}
	virtual void visit(Device&) override {}
	virtual void visit(MachineClass&) override {}
	virtual void visit(MachineDefinition&) override {}
	virtual void visit(MachineInstantiation&) override {}
	virtual void visit(IncludeLine&) override {}
	virtual void visit(TranslationUnit&) override {}

public:
	static StmtCG run(CGContext& cgc, ir::BasicBlock* block, Stmt& s)
	{
		StmtCG cg(cgc, block);

		s.accept(cg);
		return cg;
	}

	ir::BasicBlock* finalBlock() { return _inBlock; }
	bool terminated() { return !_inBlock; }
};

void StmtCG::visit(AssignStmt& a)
{
	auto val = eval(a.value(), a.targetDecl()->type());

	if (cgc.memMap.has(a.targetDecl()))
		_inBlock->append(ir::StoreInsn(cgc.memMap.addressOf(a.targetDecl()), val));
	else
		cgc.valueForDecl[a.targetDecl()] = val;
}

void StmtCG::visit(WriteStmt& w)
{
	if (w.target().device() != cgc.forDev)
		return;

	auto* ep = static_cast<Endpoint*>(w.target().endpoint());

	auto* val = eval(w.value(), ep->type());
	_inBlock->append(ir::WriteInsn(ep->eid(), val));
}

void StmtCG::visit(IfStmt& i)
{
	std::map<const Declaration*, const ir::Value*> preForkValues = cgc.valueForDecl;
	std::map<const Declaration*, const ir::Value*> declValuesInTrue, declValuesInFalse;

	auto oldBlock = _inBlock;
	auto val = eval(i.condition(), Type::Bool);
	auto trueEntry = cgc.newBlock();
	auto falseEntry = i.ifFalse() ? cgc.newBlock() : nullptr;
	ir::BasicBlock* next = nullptr;

	auto allocOrNext = [&] () {
		if (!next)
			next = cgc.newBlock();
		return next;
	};

	ir::BasicBlock* trueExit;
	ir::BasicBlock* falseExit;

	auto trueCG = run(cgc, trueEntry, i.ifTrue());
	declValuesInTrue = cgc.valueForDecl;
	if (!trueCG.terminated())
		trueCG.finalBlock()->append(ir::JumpInsn(allocOrNext()));
	trueExit = trueCG.finalBlock();

	if (i.ifFalse()) {
		auto falseCG = run(cgc, falseEntry, *i.ifFalse());
		declValuesInFalse = cgc.valueForDecl;
		if (!falseCG.terminated())
			falseCG.finalBlock()->append(ir::JumpInsn(allocOrNext()));
		falseExit = falseCG.finalBlock();
	}

	_inBlock->append(ir::SwitchInsn(val, { { 1, trueEntry } }, falseEntry ? falseEntry : allocOrNext()));

	_inBlock = next;
	if (terminated())
		return;

	if (falseEntry)
		unifyIncomingUpdates(preForkValues, { { trueExit, declValuesInTrue }, { falseExit, declValuesInFalse } });
	else
		unifyIncomingUpdates(preForkValues, { { trueExit, declValuesInTrue }, { oldBlock, {} } });
}

void StmtCG::visit(SwitchStmt& s)
{
	std::map<const Declaration*, const ir::Value*> preForkValues = cgc.valueForDecl;
	std::map<const ir::BasicBlock*, std::map<const Declaration*, const ir::Value*>> declUpdates;

	auto val = eval(s.expr(), s.expr().type());
	ir::BasicBlock* next = nullptr;

	auto allocOrNext = [&] () {
		if (!next)
			next = cgc.newBlock();
		return next;
	};

	ir::SwitchInsn::labels_type labels;
	const ir::BasicBlock* defaultLabel;

	for (auto& entry : s.entries()) {
		auto entryBlock = cgc.newBlock();

		cgc.valueForDecl = preForkValues;
		auto entryCG = run(cgc, entryBlock, entry.statement());

		if (!entryCG.terminated())
			entryCG.finalBlock()->append(ir::JumpInsn(allocOrNext()));
		declUpdates.insert({ entryCG.finalBlock(), cgc.valueForDecl });

		for (auto& label : entry.labels()) {
			if (label.expr())
				labels.insert({ label.expr()->constexprValue(), entryBlock });
			else
				defaultLabel = entryBlock;
		}
	}

	_inBlock->append(ir::SwitchInsn(val, std::move(labels), defaultLabel ? defaultLabel : allocOrNext()));

	_inBlock = next;
	if (terminated())
		return;

	unifyIncomingUpdates(preForkValues, declUpdates);
}

void StmtCG::visit(BlockStmt& b)
{
	for (auto& stmt : b.statements()) {
		auto cg = run(cgc, _inBlock, *stmt);
		_inBlock = cg.finalBlock();
		if (cg.terminated())
			break;
	}
}

void StmtCG::visit(DeclarationStmt& d)
{
	if (cgc.memMap.has(&d)) {
		auto val = eval(d.value(), d.type());
		_inBlock->append(ir::StoreInsn(cgc.memMap.addressOf(&d), val));
	} else {
		cgc.valueForDecl[&d] = eval(d.value(), d.type());
	}
}

void StmtCG::visit(GotoStmt& g)
{
	auto* nextState = _inBlock->append(ir::LoadIntInsn(cgc.newName(), ir::Type::UInt32, uint64_t(g.target()->id())));

	cgc.nextStatesByOrigin.insert({ _inBlock, nextState });
	_inBlock->append(ir::JumpInsn(cgc.exitChange));
	_inBlock = nullptr;
}



static MemoryMap initialMemoryMapFor(std::vector<MachineDefinition*> machines)
{
	MemoryMap memMap;

	for (auto* machine : machines) {
		for (auto& var : machine->variables())
			memMap.declare(var.get());

		for (auto& state : machine->states())
			for (auto& var : state.variables())
				memMap.declare(&var);
	}

	return memMap;
}

struct IRBlock {
	ir::BasicBlock* entry;
	ir::BasicBlock* exitStay;
};

class StateCG {
private:
	ir::ModuleBuilder& mb;
	MemoryMap& memMap;

	const Device* dev;
	MachineDefinition* machine;
	State& _state;
	std::string prefix;

	std::vector<OnSimpleBlock*> onEntryBlocks, onExitBlocks, onPeriodicBlocks;
	std::vector<OnExprBlock*> onExprBlocks;
	std::vector<OnUpdateBlock*> onUpdateBlocks;

	IRBlock _onEntryIR, _onPeriodicIR, _onExprIR, _onExitIR, _onUpdateIR, _alwaysIR;

	const ir::BasicBlock* exitChange;
	std::map<const ir::BasicBlock*, const ir::Value*> nextStatesByOrigin;

	void collectOnBlocks();
	template<typename Block>
	void emitOnBlocks(IRBlock& irDesc, std::vector<Block*>& blocks, std::string id);

	ir::BasicBlock* emitOnBlock(CGContext& cgc, OnSimpleBlock* on, ir::BasicBlock* block);
	ir::BasicBlock* emitOnBlock(CGContext& cgc, OnExprBlock* on, ir::BasicBlock* block);
	ir::BasicBlock* emitOnBlock(CGContext& cgc, OnUpdateBlock* on, ir::BasicBlock* block);

	void emitOnEntry();
	void emitAlways();
	void emitOnExit();

	void run();

private:
	StateCG(ir::ModuleBuilder& mb, MemoryMap& memMap, const Device* dev, MachineDefinition* machine, State& state,
			const ir::BasicBlock* exitChange)
		: mb(mb), memMap(memMap), dev(dev), machine(machine), _state(state),
		  prefix(machine->name().name() + "." + state.name().name()), exitChange(exitChange)
	{}

public:
	static StateCG run(ir::ModuleBuilder& mb, MemoryMap& memMap, const Device* dev, MachineDefinition* machine,
			State& state, const ir::BasicBlock* exitChange)
	{
		StateCG cg(mb, memMap, dev, machine, state, exitChange);

		cg.run();
		return cg;
	}

	State& state() { return _state; }
	IRBlock onEntryIR() { return _onEntryIR; }
	IRBlock onPeriodicIR() { return _onPeriodicIR; }
	IRBlock onExprIR() { return _onExprIR; }
	IRBlock onExitIR() { return _onExitIR; }
	IRBlock onUpdateIR() { return _onUpdateIR; }
	IRBlock alwaysIR() { return _alwaysIR; }
	const std::map<const ir::BasicBlock*, const ir::Value*>& nextStates() { return nextStatesByOrigin; }
};

void StateCG::collectOnBlocks()
{
	for (auto& o : _state.onBlocks()) {
		if (auto* simple = dynamic_cast<OnSimpleBlock*>(o.get())) {
			switch (simple->trigger()) {
			case OnSimpleTrigger::Entry: onEntryBlocks.push_back(simple); break;
			case OnSimpleTrigger::Exit: onExitBlocks.push_back(simple); break;
			case OnSimpleTrigger::Periodic: onPeriodicBlocks.push_back(simple); break;
			}
			continue;
		}

		if (auto* expr = dynamic_cast<OnExprBlock*>(o.get())) {
			onExprBlocks.push_back(expr);
			continue;
		}

		if (auto* update = dynamic_cast<OnUpdateBlock*>(o.get())) {
			onUpdateBlocks.push_back(update);
			continue;
		}

		throw "on block?";
	}
}

template<typename Block>
void StateCG::emitOnBlocks(IRBlock& irDesc, std::vector<Block*>& blocks, std::string id)
{
	irDesc.entry = mb.newBlock(prefix + "." + id);
	NameGenerator ng(irDesc.entry->name() + ".");
	CGContext cgc{mb, memMap, ng, dev, exitChange};

	irDesc.exitStay = irDesc.entry;
	for (auto* block : blocks)
		irDesc.exitStay = emitOnBlock(cgc, block, irDesc.exitStay);

	nextStatesByOrigin.insert(cgc.nextStatesByOrigin.begin(), cgc.nextStatesByOrigin.end());
}

ir::BasicBlock* StateCG::emitOnBlock(CGContext& cgc, OnSimpleBlock* on, ir::BasicBlock* block)
{
	return StmtCG::run(cgc, block, on->block()).finalBlock();
}

ir::BasicBlock* StateCG::emitOnBlock(CGContext& cgc, OnExprBlock* on, ir::BasicBlock* block)
{
	auto guard = cgc.newBlock();
	auto content = cgc.newBlock();
	auto next = cgc.newBlock();

	auto condCG = ExprCG::run(cgc, guard, on->condition(), Type::Bool, next);
	condCG.finalBlock()->append(ir::SwitchInsn(condCG.finalValue(), { { 1, content } }, next));

	auto blockCG = StmtCG::run(cgc, content, on->block());

	if (blockCG.terminated()) {
		block->append(ir::JumpInsn(guard));
	} else {
		auto guardVal = block->append(ir::LoadInsn(cgc.newName(), ir::Type::Bool, cgc.memMap.guardFor(on)));
		block->append(ir::SwitchInsn(guardVal, { { 0, guard } }, next));

		auto pos = content->instructions().cbegin();
		auto one = content->insert(pos, ir::LoadIntInsn(cgc.newName(), ir::Type::Bool, 1));
		content->insert(pos, ir::StoreInsn(cgc.memMap.guardFor(on), one));

		blockCG.finalBlock()->append(ir::JumpInsn(next));
	}

	return next;
}

ir::BasicBlock* StateCG::emitOnBlock(CGContext& cgc, OnUpdateBlock* on, ir::BasicBlock* block)
{
	const Device* dev = static_cast<Device*>(on->endpoint().device());
	const Endpoint* ep = static_cast<Endpoint*>(on->endpoint().endpoint());

	auto* matches = cgc.newBlock();
	auto* cont = cgc.newBlock();

	auto* cmpIP = dev == cgc.forDev
		? block->append(ir::CompareIPInsn(cgc.newName(), 0, 16, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}))
		: block->append(ir::CompareIPInsn(cgc.newName(), 0, 16, dev->address()));
	auto* eid = block->append(ir::LoadIntInsn(cgc.newName(), ir::Type::UInt32, ep->eid()));
	auto* srcEID = block->append(ir::LoadSpecialInsn(cgc.newName(), ir::Type::UInt32, ir::SpecialVal::SourceEID));
	auto* cmpEID = block->append(ir::ArithmeticInsn(cgc.newName(), ir::Type::Bool, ir::ArithOp::Eq, eid, srcEID));
	auto* cmp = block->append(ir::ArithmeticInsn(cgc.newName(), ir::Type::Bool, ir::ArithOp::And, cmpIP, cmpEID));
	block->append(ir::SwitchInsn(cmp, ir::SwitchInsn::labels_type{ { 1, matches } }, cont));

	auto cg = StmtCG::run(cgc, matches, on->block());
	if (!cg.terminated())
		cg.finalBlock()->append(ir::JumpInsn(cont));

	return cont;
}

void StateCG::emitOnEntry()
{
	emitOnBlocks(_onEntryIR, onEntryBlocks, "entry");

	NameGenerator ng(prefix + ".entryVarSet");
	CGContext cgc{mb, memMap, ng, dev, exitChange};

	auto stateInitHead = cgc.newBlock();
	auto stateInit = stateInitHead;
	for (auto& var : _state.variables())
		stateInit = StmtCG::run(cgc, stateInit, var).finalBlock();

	stateInit->append(ir::JumpInsn(_onEntryIR.entry));
	_onEntryIR.entry = stateInitHead;
}

void StateCG::emitAlways()
{
	NameGenerator ng(prefix + ".always");
	CGContext cgc{mb, memMap, ng, dev, exitChange};

	_alwaysIR.entry = cgc.newBlock();
	if (_state.always())
		_alwaysIR.exitStay = StmtCG::run(cgc, _alwaysIR.entry, *_state.always()).finalBlock();
	else
		_alwaysIR.exitStay = _alwaysIR.entry;

	nextStatesByOrigin.insert(cgc.nextStatesByOrigin.begin(), cgc.nextStatesByOrigin.end());
}

void StateCG::emitOnExit()
{
	emitOnBlocks(_onExitIR, onExitBlocks, "exit");

	NameGenerator ng(prefix + ".exit");
	CGContext cgc{mb, memMap, ng, dev, exitChange};

	auto* zero = _onExitIR.exitStay->append(ir::LoadIntInsn(cgc.newName(), ir::Type::Bool, 0));
	for (auto o : onExprBlocks) {
		if (memMap.hasGuardFor(o))
			_onExitIR.exitStay->append(ir::StoreInsn(memMap.guardFor(o), zero));
	}
}

void StateCG::run()
{
	collectOnBlocks();
	emitOnEntry();
	emitOnBlocks(_onPeriodicIR, onPeriodicBlocks, "periodic");
	emitOnBlocks(_onExprIR, onExprBlocks, "expr");
	emitOnBlocks(_onUpdateIR, onUpdateBlocks, "update");
	emitAlways();
	emitOnExit();
}



class MachineCG {
private:
	ir::ModuleBuilder& mb;
	MemoryMap& memMap;

	const Device* dev;
	MachineDefinition* machine;
	std::string prefix;
	const ir::Value* isPeriodicCheck;

	std::vector<StateCG> states;
	IRBlock _onInit;

	ir::BasicBlock* exitChange;
	ir::BasicBlock* _machineHead;
	ir::BasicBlock* _machineTail;

	void emitOnInit();
	void emitStateGlue();
	void run();

private:
	MachineCG(ir::ModuleBuilder& mb, MemoryMap& memMap, const Device* dev, MachineDefinition* machine,
			const ir::Value* isPeriodicCheck)
		: mb(mb), memMap(memMap), dev(dev), machine(machine), prefix(machine->name().name()),
		  isPeriodicCheck(isPeriodicCheck)
	{}

public:
	static MachineCG run(ir::ModuleBuilder& mb, MemoryMap& memMap, const Device* dev, MachineDefinition* machine,
			const ir::Value* isPeriodicCheck)
	{
		MachineCG cg(mb, memMap, dev, machine, isPeriodicCheck);

		cg.run();
		return cg;
	}

	IRBlock onInit() { return _onInit; }
	ir::BasicBlock* machineHead() { return _machineHead; }
	ir::BasicBlock* machineTail() { return _machineTail; }
};

void MachineCG::emitOnInit()
{
	NameGenerator ng(prefix + ".init");
	CGContext cgc{mb, memMap, ng, dev, nullptr};

	_onInit.entry = cgc.newBlock();
	_onInit.exitStay = _onInit.entry;

	for (auto& var : machine->variables())
		_onInit.exitStay = StmtCG::run(cgc, _onInit.exitStay, *var).finalBlock();
	if (machine->states().size())
		for (auto& var : machine->states()[0].variables())
			_onInit.exitStay = StmtCG::run(cgc, _onInit.exitStay, var).finalBlock();
}

void MachineCG::emitStateGlue()
{
	_machineHead = mb.newBlock(prefix + ".head");
	_machineTail = mb.newBlock(prefix + ".tail");
	auto changeAfterExit = mb.newBlock(prefix + ".changeState2");

	ir::SwitchInsn::labels_type stateLabels, stateEntryLabels, stateExitLabels;
	ir::PhiInsn::sources_type newStateSources;

	for (auto& state : states) {
		auto stateHead = mb.newBlock(str(boost::format("%1%.%2%.head") % prefix % state.state().id()));

		stateLabels.insert({ state.state().id(), stateHead });
		stateHead->append(ir::SwitchInsn(
			isPeriodicCheck,
			{
				{ 0, state.onUpdateIR().entry },
				{ 1, state.onPeriodicIR().entry }
			},
			nullptr));

		state.onPeriodicIR().exitStay->append(ir::JumpInsn(state.onExprIR().entry));
		state.onExprIR().exitStay->append(ir::JumpInsn(state.alwaysIR().entry));
		state.onUpdateIR().exitStay->append(ir::JumpInsn(state.alwaysIR().entry));
		state.alwaysIR().exitStay->append(ir::JumpInsn(_machineTail));

		stateEntryLabels.insert({ state.state().id(), state.onEntryIR().entry });
		stateExitLabels.insert({ state.state().id(), state.onExitIR().entry });

		state.onExitIR().exitStay->append(ir::JumpInsn(changeAfterExit));
		state.onEntryIR().exitStay->append(ir::JumpInsn(_machineTail));

		newStateSources.insert(state.nextStates().begin(), state.nextStates().end());
	}

	auto stateVar = memMap.newAnonymous(Type::UInt32);
	auto curState = _machineHead->append(ir::LoadInsn(prefix + ".state", ir::Type::UInt32, stateVar));
	auto lastPossibleState = stateLabels[states.back().state().id()];
	stateLabels.erase(states.back().state().id());
	_machineHead->append(ir::SwitchInsn(curState, std::move(stateLabels), lastPossibleState));

	auto next = exitChange->append(ir::PhiInsn(prefix + ".newstate", ir::Type::UInt32, std::move(newStateSources)));
	exitChange->append(ir::SwitchInsn(curState, std::move(stateExitLabels), changeAfterExit));
	changeAfterExit->append(ir::StoreInsn(stateVar, next));
	changeAfterExit->append(ir::SwitchInsn(next, std::move(stateEntryLabels), _machineTail));
}

void MachineCG::run()
{
	exitChange = mb.newBlock(prefix + ".changeState");

	for (auto& state : machine->states())
		states.emplace_back(StateCG::run(mb, memMap, dev, machine, state, exitChange));

	emitOnInit();
	emitStateGlue();
}

}

std::unique_ptr<ir::ModuleBuilder> generateIR(const Device* dev, std::set<MachineDefinition*>& machines)
{
	std::vector<MachineDefinition*> sortedMachines(machines.begin(), machines.end());
	std::sort(sortedMachines.begin(), sortedMachines.end(), [] (MachineDefinition* a, MachineDefinition* b) {
		return a->name().name() < b->name().name();
	});

	std::unique_ptr<ir::ModuleBuilder> moduleBuilder(new ir::ModuleBuilder(0));
	MemoryMap memMap(initialMemoryMapFor(sortedMachines));

	ir::BasicBlock* onInit = moduleBuilder->newBlock("onInit");
	ir::BasicBlock* onPacket = moduleBuilder->newBlock("onPacket");
	ir::BasicBlock* onPeriodic = moduleBuilder->newBlock("onPeriodic");
	ir::BasicBlock* onEvent = moduleBuilder->newBlock("onEvent");

	moduleBuilder->onInit(onInit);
	moduleBuilder->onPacket(onPacket);
	moduleBuilder->onPeriodic(onPeriodic);

	auto* inPacket = onPacket->append(ir::LoadIntInsn("isPeriodic0", ir::Type::Bool, 0L));
	auto* inPeriodic = onPeriodic->append(ir::LoadIntInsn("isPeriodic1", ir::Type::Bool, 1L));

	onPacket->append(ir::JumpInsn(onEvent));
	onPeriodic->append(ir::JumpInsn(onEvent));

	auto* isPeriodicCheck = onEvent->append(ir::PhiInsn(
		"isPeriodicCheck",
		ir::Type::Bool,
		ir::PhiInsn::sources_type{ {onPacket, inPacket}, {onPeriodic, inPeriodic} }));

	std::vector<MachineCG> machineIR;
	for (auto machine : sortedMachines)
		machineIR.emplace_back(MachineCG::run(*moduleBuilder, memMap, dev, machine, isPeriodicCheck));

	for (auto& mir : machineIR) {
		onInit->append(ir::JumpInsn(mir.onInit().entry));
		onInit = mir.onInit().exitStay;

		onEvent->append(ir::JumpInsn(mir.machineHead()));
		onEvent = mir.machineTail();
	}
	onInit->append(ir::ReturnInsn());
	onEvent->append(ir::ReturnInsn());

	return moduleBuilder;
}

}
}
