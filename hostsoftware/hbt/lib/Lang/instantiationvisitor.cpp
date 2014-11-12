#include "instantiationvisitor.hpp"

namespace hbt {
namespace lang {

std::unique_ptr<Expr> InstantiationVisitor::clone(Expr& e)
{
	std::unique_ptr<Expr> result;
	e.accept(*this);
	swap(result, _expr);
	return std::move(result);
}

std::unique_ptr<Stmt> InstantiationVisitor::clone(Stmt& s)
{
	std::unique_ptr<Stmt> result;
	s.accept(*this);
	swap(result, _stmt);
	return std::move(result);
}

std::unique_ptr<OnBlock> InstantiationVisitor::clone(OnBlock& o)
{
	std::unique_ptr<OnBlock> result;
	o.accept(*this);
	swap(result, _onBlock);
	return std::move(result);
}

void InstantiationVisitor::visit(IdentifierExpr& i)
{
	if (i.value()) {
		_expr = clone(*i.value());
	} else if (auto* decl = dynamic_cast<DeclarationStmt*>(i.target())) {
		std::unique_ptr<IdentifierExpr> ic(new auto(i));
		ic->target(declClones.at(decl));
		_expr = std::move(ic);
	} else {
		_expr.reset(new auto(i));
	}
}

void InstantiationVisitor::visit(TypedLiteral<bool>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<uint8_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<uint16_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<uint32_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<uint64_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<int8_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<int16_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<int32_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<int64_t>& l) { _expr.reset(new auto(l)); }
void InstantiationVisitor::visit(TypedLiteral<float>& l) { _expr.reset(new auto(l)); }

void InstantiationVisitor::visit(CastExpr& c)
{
	if (c.typeSource())
		_expr.reset(new CastExpr(c.sloc(), clone(*c.typeSource()), clone(c.expr())));
	else
		_expr.reset(new CastExpr(c.sloc(), c.type(), clone(c.expr())));
}

void InstantiationVisitor::visit(UnaryExpr& u)
{
	_expr.reset(new UnaryExpr(u.sloc(), u.op(), clone(u.expr())));
}

void InstantiationVisitor::visit(BinaryExpr& b)
{
	_expr.reset(new BinaryExpr(b.sloc(), clone(b.left()), b.op(), clone(b.right())));
}

void InstantiationVisitor::visit(ConditionalExpr& c)
{
	_expr.reset(new ConditionalExpr(c.sloc(), clone(c.condition()), clone(c.ifTrue()), clone(c.ifFalse())));
}

void InstantiationVisitor::visit(EndpointExpr& e)
{
	_expr.reset(new auto(e));
}

void InstantiationVisitor::visit(CallExpr& c)
{
	std::vector<std::unique_ptr<Expr>> args;

	for (auto& arg : c.arguments())
		args.emplace_back(clone(*arg));

	_expr.reset(new CallExpr(c.sloc(), c.name(), std::move(args), c.type()));
	static_cast<CallExpr&>(*_expr).target(c.target());
}

void InstantiationVisitor::visit(AssignStmt& a)
{
	std::unique_ptr<AssignStmt> ac(new AssignStmt(a.sloc(), a.target(), clone(a.value())));
	ac->targetDecl(declClones.at(a.targetDecl()));
	_stmt = std::move(ac);
}

void InstantiationVisitor::visit(WriteStmt& w)
{
	_stmt.reset(new WriteStmt(w.sloc(), static_cast<EndpointExpr&>(*clone(w.target())), clone(w.value())));
}

void InstantiationVisitor::visit(IfStmt& i)
{
	std::unique_ptr<Stmt> ifFalse;

	if (i.ifFalse())
		ifFalse = clone(*i.ifFalse());

	_stmt.reset(new IfStmt(i.sloc(), clone(i.condition()), clone(i.ifTrue()), std::move(ifFalse)));
}

void InstantiationVisitor::visit(SwitchStmt& s)
{
	std::vector<SwitchEntry> entries;

	for (auto& entry : s.entries()) {
		std::vector<SwitchLabel> labels;

		for (auto& label : entry.labels()) {
			if (label.expr())
				labels.emplace_back(label.sloc(), clone(*label.expr()));
			else
				labels.emplace_back(label.sloc(), nullptr);
		}

		entries.emplace_back(std::move(labels), clone(entry.statement()));
	}

	_stmt.reset(new SwitchStmt(s.sloc(), clone(s.expr()), std::move(entries)));
}

void InstantiationVisitor::visit(BlockStmt& b)
{
	std::vector<std::unique_ptr<Stmt>> stmts;

	for (auto& s : b.statements())
		stmts.push_back(clone(*s));

	_stmt.reset(new BlockStmt(b.sloc(), std::move(stmts)));
}

void InstantiationVisitor::visit(DeclarationStmt& d)
{
	std::unique_ptr<DeclarationStmt> dc;

	if (d.typeSource())
		dc.reset(new DeclarationStmt(d.sloc(), clone(*d.typeSource()), d.name(), clone(d.value())));
	else
		dc.reset(new DeclarationStmt(d.sloc(), d.type(), d.name(), clone(d.value())));

	declClones[&d] = dc.get();
	_stmt = std::move(dc);
}

void InstantiationVisitor::visit(GotoStmt& g)
{
	_stmt.reset(new auto(g));
}

void InstantiationVisitor::visit(OnSimpleBlock& o)
{
	std::unique_ptr<BlockStmt> block(static_cast<BlockStmt*>(clone(o.block()).release()));
	_onBlock.reset(new OnSimpleBlock(o.sloc(), o.trigger(), std::move(block)));
}

void InstantiationVisitor::visit(OnExprBlock& o)
{
	std::unique_ptr<BlockStmt> block(static_cast<BlockStmt*>(clone(o.block()).release()));
	_onBlock.reset(new OnExprBlock(o.sloc(), clone(o.condition()), std::move(block)));
}

void InstantiationVisitor::visit(OnUpdateBlock& o)
{
	std::unique_ptr<BlockStmt> block(static_cast<BlockStmt*>(clone(o.block()).release()));
	std::unique_ptr<EndpointExpr> ep(static_cast<EndpointExpr*>(clone(o.endpoint()).release()));
	_onBlock.reset(new OnUpdateBlock(o.sloc(), std::move(*ep), std::move(block)));
}

void InstantiationVisitor::visit(Endpoint&) {}
void InstantiationVisitor::visit(Device&) {}

State InstantiationVisitor::clone(State& s)
{
	std::vector<std::unique_ptr<DeclarationStmt>> decls;
	std::vector<std::unique_ptr<OnBlock>> onBlocks;

	for (auto& decl : s.variables())
		decls.emplace_back(cloneCast<DeclarationStmt>(*decl));
	for (auto& on : s.onBlocks())
		onBlocks.push_back(clone(*on));

	State res(s.sloc(), s.name(), std::move(decls), std::move(onBlocks));
	res.id(s.id());
	return res;
}

void InstantiationVisitor::visit(MachineClass&) {}
void InstantiationVisitor::visit(MachineDefinition&) {}

void InstantiationVisitor::visit(MachineInstantiation& m)
{
	std::vector<std::unique_ptr<DeclarationStmt>> decls;
	std::vector<State> states;

	for (auto& decl : m.baseClass()->variables())
		decls.emplace_back(cloneCast<DeclarationStmt>(*decl));

	for (auto& state : m.baseClass()->states())
		states.emplace_back(clone(state));

	m.instantiation(
		std::unique_ptr<MachineDefinition>(
			new MachineDefinition(m.sloc(), m.name(), std::move(decls), std::move(states))));
}

void InstantiationVisitor::visit(BehaviourClass&) {}
void InstantiationVisitor::visit(BehaviourDefinition&) {}

void InstantiationVisitor::visit(BehaviourInstantiation& b)
{
	std::vector<std::unique_ptr<DeclarationStmt>> decls;
	std::vector<std::unique_ptr<SyntheticEndpoint>> endpoints;
	std::vector<std::unique_ptr<OnBlock>> onBlocks;

	for (auto& decl : b.baseClass()->variables())
		decls.emplace_back(cloneCast<DeclarationStmt>(*decl));

	for (auto& ep : b.baseClass()->endpoints()) {
		auto readBlock = ep->readBlock() ? cloneCast<BlockStmt>(*ep->readBlock()) : nullptr;
		auto readValue = ep->readValue() ? clone(*ep->readValue()) : nullptr;
		auto write = ep->write() ? cloneCast<BlockStmt>(*ep->write()) : nullptr;

		std::unique_ptr<SyntheticEndpoint> cloned(
			new SyntheticEndpoint(
				ep->sloc(), ep->name(), ep->type(), std::move(readBlock), std::move(readValue), std::move(write)));
		endpoints.emplace_back(std::move(cloned));
	}

	for (auto& on : b.baseClass()->onBlocks())
		onBlocks.emplace_back(clone(*on));

	b.instantiation(
		std::unique_ptr<BehaviourDefinition>(
			new BehaviourDefinition(
				b.sloc(), b.name(), b.device(), std::move(decls), std::move(endpoints), std::move(onBlocks))));
}

void InstantiationVisitor::visit(IncludeLine&) {}
void InstantiationVisitor::visit(TranslationUnit&) {}

}
}
