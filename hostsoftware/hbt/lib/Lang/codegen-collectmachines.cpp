#include "hbt/Lang/codegen.hpp"

#include "hbt/Lang/ast.hpp"

namespace hbt {
namespace lang {

namespace {

struct CollectVisitor : ASTVisitor {
	std::map<const Device*, std::set<MachineDefinition*>> machines;
	MachineDefinition* currentMachine;

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

	virtual void visit(AssignStmt&) override {}
	virtual void visit(WriteStmt&) override;
	virtual void visit(IfStmt&) override;
	virtual void visit(SwitchStmt&) override;
	virtual void visit(BlockStmt&) override;
	virtual void visit(DeclarationStmt&) override {}
	virtual void visit(GotoStmt&) override {}

	virtual void visit(OnSimpleBlock&) override;
	virtual void visit(OnExprBlock&) override;
	virtual void visit(OnUpdateBlock&) override;

	virtual void visit(Endpoint&) override {}
	virtual void visit(Device&) override {}
	virtual void visit(MachineClass&) override {}
	virtual void visit(MachineDefinition&) override;
	virtual void visit(MachineInstantiation&) override;
	virtual void visit(BehaviourClass&) override {}
	virtual void visit(BehaviourDefinition&) override {}
	virtual void visit(BehaviourInstantiation&) override {}
	virtual void visit(IncludeLine&) override {}
	virtual void visit(TranslationUnit&) override;
};

void CollectVisitor::visit(WriteStmt& w)
{
	auto* dev = static_cast<const Device*>(w.target().device());
	machines[dev].insert(currentMachine);
}

void CollectVisitor::visit(IfStmt& i)
{
	i.ifTrue().accept(*this);
	if (i.ifFalse())
		i.ifFalse()->accept(*this);
}

void CollectVisitor::visit(SwitchStmt& s)
{
	for (auto& entry : s.entries())
		entry.statement().accept(*this);
}

void CollectVisitor::visit(BlockStmt& b)
{
	for (auto& stmt : b.statements())
		stmt->accept(*this);
}

void CollectVisitor::visit(OnSimpleBlock& o)
{
	o.block().accept(*this);
}

void CollectVisitor::visit(OnExprBlock& o)
{
	o.block().accept(*this);
}

void CollectVisitor::visit(OnUpdateBlock& o)
{
	o.block().accept(*this);
}

void CollectVisitor::visit(MachineDefinition& m)
{
	currentMachine = &m;
	for (auto& state : m.states()) {
		for (auto& ob : state.onBlocks())
			ob->accept(*this);
	}
}

void CollectVisitor::visit(MachineInstantiation& m)
{
	m.instantiation()->accept(*this);
}

void CollectVisitor::visit(TranslationUnit& t)
{
	for (auto& part : t.items())
		part->accept(*this);
}

}

std::map<const Device*, std::set<MachineDefinition*>> collectMachines(TranslationUnit& tu)
{
	CollectVisitor cv;

	tu.accept(cv);
	return cv.machines;
}

}
}
