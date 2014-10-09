#ifndef INCLUDE_LANG_ASTPRINTER_HPP_E11315AAB751B6DC
#define INCLUDE_LANG_ASTPRINTER_HPP_E11315AAB751B6DC

#include <ostream>

#include "Lang/ast.hpp"

namespace hbt {
namespace lang {

class ASTPrinter : public ASTVisitor {
private:
	unsigned _indent;
	bool _skipIndent;
	std::ostream& out;

	void indent();

	void printState(State& s);
	void printOnBlockBody(OnBlock& o);
	void printMachineBody(MachineBody& m);

public:
	ASTPrinter(std::ostream& out)
		: _indent(0), _skipIndent(false), out(out)
	{}

	virtual void visit(IdentifierExpr& i) override;
	virtual void visit(TypedLiteral<bool>& l) override;
	virtual void visit(TypedLiteral<uint8_t>& l) override;
	virtual void visit(TypedLiteral<uint16_t>& l) override;
	virtual void visit(TypedLiteral<uint32_t>& l) override;
	virtual void visit(TypedLiteral<uint64_t>& l) override;
	virtual void visit(TypedLiteral<int8_t>& l) override;
	virtual void visit(TypedLiteral<int16_t>& l) override;
	virtual void visit(TypedLiteral<int32_t>& l) override;
	virtual void visit(TypedLiteral<int64_t>& l) override;
	virtual void visit(TypedLiteral<float>& l) override;
	virtual void visit(CastExpr& c) override;
	virtual void visit(UnaryExpr& u) override;
	virtual void visit(BinaryExpr& b) override;
	virtual void visit(ConditionalExpr& c) override;
	virtual void visit(EndpointExpr& e) override;
	virtual void visit(CallExpr& c) override;
	virtual void visit(PacketEIDExpr& p) override;
	virtual void visit(PacketValueExpr& p) override;
	virtual void visit(SysTimeExpr& p) override;
	virtual void visit(TimeoutExpr& t) override;

	virtual void visit(AssignStmt& a) override;
	virtual void visit(WriteStmt& w) override;
	virtual void visit(IfStmt& i) override;
	virtual void visit(SwitchStmt& s) override;
	virtual void visit(BlockStmt& b) override;
	virtual void visit(DeclarationStmt& d) override;
	virtual void visit(GotoStmt& g) override;

	virtual void visit(OnSimpleBlock& o) override;
	virtual void visit(OnPacketBlock& o) override;
	virtual void visit(OnExprBlock& o) override;

	virtual void visit(Endpoint& e) override;
	virtual void visit(Device& d) override;
	virtual void visit(MachineClass& m) override;
	virtual void visit(MachineDefinition& m) override;
	virtual void visit(MachineInstantiation& m) override;
	virtual void visit(IncludeLine& i) override;
	virtual void visit(TranslationUnit& t) override;
};

}
}

#endif
