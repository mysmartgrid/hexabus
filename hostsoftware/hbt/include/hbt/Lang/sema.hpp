#ifndef INCLUDE_LANG_SEMA_HPP_4EA264F49A258110
#define INCLUDE_LANG_SEMA_HPP_4EA264F49A258110

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "hbt/Lang/ast.hpp"
#include "hbt/Lang/diagnostics.hpp"
#include "hbt/Lang/sema-scope.hpp"
#include "hbt/Lang/type.hpp"

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
			struct {
				Declaration* behaviour;
				EndpointDeclaration* endpoint;
			} ep;
		};

		ClassParamInstance(ClassParameter& cp)
			: parameter(cp), hasValue(false), used(false)
		{}

		ClassParamInstance(ClassParameter& cp, Expr* value)
			: parameter(cp), hasValue(true), used(false), value(value)
		{}

		ClassParamInstance(ClassParameter& cp, Device* device)
			: parameter(cp), hasValue(true), used(false), device(device)
		{}

		ClassParamInstance(ClassParameter& cp, Declaration* behaviour, EndpointDeclaration* ep)
			: parameter(cp), hasValue(true), used(false), ep{behaviour, ep}
		{}
	};

private:
	DiagnosticOutput& diags;

	std::map<uint32_t, Endpoint*> endpointsByEID;

	std::map<std::string, State*> knownStates;

	std::map<std::string, ClassParamInstance> classParams;

	Scope globalScope;
	Scope* currentScope;

	EndpointExpr* liveEndpoint;

	const char* gotoExclusionScope;
	Device* behaviourDevice;

	bool isResolvingType, inBehaviour;

	void declareInCurrentScope(Declaration& decl);

	void checkState(State& s);
	void checkMachineBody(MachineBody& m);
	bool checkBehaviourBody(Behaviour& b, Device* dev);
	bool resolveType(Expr& e);

	Declaration* resolveDevice(EndpointExpr& e);
	Declaration* resolveEndpointExpr(EndpointExpr& e, bool incompletePath);

	EndpointDeclaration* checkEndpointExpr(EndpointExpr& e, bool forRead);

	bool declareClassParams(std::vector<std::unique_ptr<ClassParameter>>& params, bool forbidContextualIdentifiers);
	bool instantiateClassParams(const SourceLocation& sloc, const Identifier& className,
			std::vector<std::unique_ptr<ClassParameter>>& params, std::vector<std::unique_ptr<Expr>>& args);

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

	virtual void visit(AssignStmt& a) override;
	virtual void visit(WriteStmt& w) override;
	virtual void visit(IfStmt& i) override;
	virtual void visit(SwitchStmt& s) override;
	virtual void visit(BlockStmt& b) override;
	virtual void visit(DeclarationStmt& d) override;
	virtual void visit(GotoStmt& g) override;

	virtual void visit(OnSimpleBlock& o) override;
	virtual void visit(OnExprBlock& o) override;
	virtual void visit(OnUpdateBlock& o) override;

	virtual void visit(Endpoint& e) override;
	virtual void visit(Device& d) override;
	virtual void visit(MachineClass& m) override;
	virtual void visit(MachineDefinition& m) override;
	virtual void visit(MachineInstantiation& m) override;
	virtual void visit(BehaviourClass& m) override;
	virtual void visit(BehaviourDefinition& m) override;
	virtual void visit(BehaviourInstantiation& m) override;
	virtual void visit(IncludeLine& i) override;
	virtual void visit(TranslationUnit& t) override;
};

}
}

#endif
