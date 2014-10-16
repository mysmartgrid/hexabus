#ifndef INCLUDE_LANG_SEMA_HPP_4EA264F49A258110
#define INCLUDE_LANG_SEMA_HPP_4EA264F49A258110

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "Lang/ast.hpp"
#include "Lang/diagnostics.hpp"
#include "Lang/sema-scope.hpp"
#include "Lang/type.hpp"

namespace hbt {
namespace lang {

class SemanticVisitor : public ASTVisitor {
private:
	struct ClassParamInstance {
		ClassParameter& parameter;
		const bool hasValue;
		bool used;

		union {
			Expr* value;
			Device* device;
			Endpoint* endpoint;
		};

		ClassParamInstance(ClassParameter& cp)
			: parameter(cp), hasValue(false), used(false)
		{}

		ClassParamInstance(ClassParameter& cp, Expr* value)
			: parameter(cp), hasValue(true), value(value), used(false)
		{}

		ClassParamInstance(ClassParameter& cp, Device* device)
			: parameter(cp), hasValue(true), device(device), used(false)
		{}

		ClassParamInstance(ClassParameter& cp, Endpoint* ep)
			: parameter(cp), hasValue(true), endpoint(ep), used(false)
		{}
	};

private:
	DiagnosticOutput& diags;

	std::map<uint32_t, Endpoint*> endpointsByEID;

	std::map<std::string, State*> knownStates;

	std::map<std::string, ClassParamInstance> classParams;

	std::vector<BuiltinFunction> builtinFunctions;
	Scope globalScope;
	Scope* currentScope;

	void declareInCurrentScope(Declaration& decl);

	void checkState(State& s);
	void checkMachineBody(MachineBody& m);

	std::pair<Declaration*, Device*> resolveDeviceInScope(const Identifier& device);
	std::pair<Declaration*, Endpoint*> resolveEndpointInScope(const Identifier& endpoint);

	Endpoint* checkEndpointExpr(EndpointExpr& e);

public:
	SemanticVisitor(DiagnosticOutput& diagOut);

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
