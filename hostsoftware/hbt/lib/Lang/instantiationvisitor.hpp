#ifndef LIB_LANG_INSTANTIATIONVISITOR_HPP_937EC712DE60D9BB
#define LIB_LANG_INSTANTIATIONVISITOR_HPP_937EC712DE60D9BB

#include <map>
#include <memory>

#include "Lang/ast.hpp"
#include "Lang/sema-scope.hpp"

namespace hbt {
namespace lang {

class InstantiationVisitor : public ASTVisitor {
private:
	std::unique_ptr<Expr> _expr;
	std::unique_ptr<Stmt> _stmt;
	std::unique_ptr<OnBlock> _onBlock;

	std::map<Declaration*, Expr*> cpValues;
	std::map<Declaration*, Device*> cpDevices;
	std::map<Declaration*, Endpoint*> cpEndpoints;

	std::map<Declaration*, DeclarationStmt*> declClones;

	Scope& scope;

	std::unique_ptr<Expr> clone(Expr& e);
	std::unique_ptr<Stmt> clone(Stmt& e);
	std::unique_ptr<OnBlock> clone(OnBlock& e);
	State clone(State& e);

public:
	InstantiationVisitor(Scope& scope)
		: scope(scope)
	{}

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

	virtual void visit(AssignStmt&) override;
	virtual void visit(WriteStmt&) override;
	virtual void visit(IfStmt&) override;
	virtual void visit(SwitchStmt&) override;
	virtual void visit(BlockStmt&) override;
	virtual void visit(DeclarationStmt&) override;
	virtual void visit(GotoStmt&) override;

	virtual void visit(OnSimpleBlock&) override;
	virtual void visit(OnExprBlock&) override;
	virtual void visit(OnUpdateBlock&) override;

	virtual void visit(Endpoint&) override;
	virtual void visit(Device&) override;
	virtual void visit(MachineClass&) override;
	virtual void visit(MachineDefinition&) override;
	virtual void visit(MachineInstantiation&) override;
	virtual void visit(IncludeLine&) override;
	virtual void visit(TranslationUnit&) override;
};

}
}

#endif
