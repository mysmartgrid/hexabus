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
	if (cpValues.count(i.target())) {
		_expr = clone(*cpValues.at(i.target()));
		_expr->type(static_cast<CPValue*>(i.target())->type());
	} else {
		std::unique_ptr<IdentifierExpr> c(new auto(i));
		c->target(declClones.at(i.target()));
		_expr = std::move(c);
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
	auto devId = cpDevices.count(e.device()) ? cpDevices.at(e.device())->name() : e.deviceId();
	auto epId = cpEndpoints.count(e.endpoint()) ? cpEndpoints.at(e.endpoint())->name() : e.endpointId();

	auto dev = cpDevices.count(e.device()) ? cpDevices.at(e.device()) : e.device();
	auto ep = cpEndpoints.count(e.endpoint()) ? cpEndpoints.at(e.endpoint()) : e.endpoint();

	std::unique_ptr<EndpointExpr> ec(new EndpointExpr(e.sloc(), devId, epId, e.type()));
	ec->device(dev);
	ec->endpoint(ep);
	_expr = std::move(ec);
}

void InstantiationVisitor::visit(CallExpr& c)
{
	std::vector<std::unique_ptr<Expr>> args;

	for (auto& arg : c.arguments())
		args.emplace_back(clone(*arg));

	_expr.reset(new CallExpr(c.sloc(), c.name(), std::move(args), c.type()));
}

void InstantiationVisitor::visit(PacketEIDExpr& p) { _expr.reset(new auto(p)); }
void InstantiationVisitor::visit(PacketValueExpr& p) { _expr.reset(new auto(p)); }
void InstantiationVisitor::visit(SysTimeExpr& s) { _expr.reset(new auto(s)); }
void InstantiationVisitor::visit(TimeoutExpr& t) { _expr.reset(new auto(t)); }

void InstantiationVisitor::visit(AssignStmt& a)
{
	_stmt.reset(new AssignStmt(a.sloc(), a.target(), clone(a.value())));
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
	std::unique_ptr<DeclarationStmt> dc(new DeclarationStmt(d.sloc(), d.type(), d.name(), clone(d.value())));

	declClones[&d] = dc.get();
	_stmt = std::move(dc);
}

void InstantiationVisitor::visit(GotoStmt& g) { _stmt.reset(new auto(g)); }

void InstantiationVisitor::visit(OnSimpleBlock& o)
{
	std::unique_ptr<BlockStmt> block(static_cast<BlockStmt*>(clone(o.block()).release()));
	_onBlock.reset(new OnSimpleBlock(o.sloc(), o.trigger(), std::move(block)));
}

void InstantiationVisitor::visit(OnPacketBlock& o)
{
	std::unique_ptr<BlockStmt> block(static_cast<BlockStmt*>(clone(o.block()).release()));
	std::unique_ptr<OnPacketBlock> c;

	if (auto* cpd = dynamic_cast<CPDevice*>(o.source())) {
		auto* src = cpDevices.at(cpd);
		c.reset(new OnPacketBlock(o.sloc(), src->name(), std::move(block)));
		c->source(src);
	} else {
		c.reset(new OnPacketBlock(o.sloc(), o.sourceId(), std::move(block)));
		c->source(o.source());
	}

	_onBlock = std::move(c);
}

void InstantiationVisitor::visit(OnExprBlock& o)
{
	std::unique_ptr<BlockStmt> block(static_cast<BlockStmt*>(clone(o.block()).release()));
	_onBlock.reset(new OnExprBlock(o.sloc(), clone(o.condition()), std::move(block)));
}

void InstantiationVisitor::visit(Endpoint&) {}
void InstantiationVisitor::visit(Device&) {}

State InstantiationVisitor::clone(State& s)
{
	std::vector<DeclarationStmt> decls;
	std::vector<std::unique_ptr<OnBlock>> onBlocks;
	std::vector<std::unique_ptr<Stmt>> statements;

	for (auto& decl : s.variables())
		decls.push_back(std::move(static_cast<DeclarationStmt&>(*clone(decl))));
	for (auto& on : s.onBlocks())
		onBlocks.push_back(clone(*on));
	for (auto& stmt : s.statements())
		statements.push_back(clone(*stmt));

	return { s.sloc(), s.name(), std::move(decls), std::move(onBlocks), std::move(statements) };
}

void InstantiationVisitor::visit(MachineClass&) {}
void InstantiationVisitor::visit(MachineDefinition&) {}

void InstantiationVisitor::visit(MachineInstantiation& m)
{
	std::vector<std::unique_ptr<DeclarationStmt>> decls;
	std::vector<State> states;

	cpValues.clear();
	cpDevices.clear();
	cpEndpoints.clear();
	declClones.clear();

	for (size_t i = 0; i < m.arguments().size(); i++) {
		auto& param = *m.baseClass()->parameters()[i];
		auto& arg = *m.arguments()[i];

		switch (param.kind()) {
		case ClassParameter::Kind::Value:
			cpValues.insert({ &static_cast<CPValue&>(param), &arg });
			break;

		case ClassParameter::Kind::Device:
			cpDevices.insert({
				&static_cast<CPDevice&>(param),
				static_cast<Device*>(scope.resolve(static_cast<IdentifierExpr&>(arg).name()))
			});
			break;

		case ClassParameter::Kind::Endpoint:
			cpEndpoints.insert({
				&static_cast<CPEndpoint&>(param),
				static_cast<Endpoint*>(scope.resolve(static_cast<IdentifierExpr&>(arg).name()))
			});
			break;
		}
	}

	for (auto& decl : m.baseClass()->variables())
		decls.emplace_back(static_cast<DeclarationStmt*>(clone(*decl).release()));

	for (auto& state : m.baseClass()->states())
		states.emplace_back(clone(state));

	m.instantiation(
		std::unique_ptr<MachineDefinition>(
			new MachineDefinition(m.sloc(), m.name(), std::move(decls), std::move(states))));
}

void InstantiationVisitor::visit(IncludeLine&) {}
void InstantiationVisitor::visit(TranslationUnit&) {}

}
}
