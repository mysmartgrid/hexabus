#ifndef INCLUDE_LANG_AST_HPP_4EAF8E6910F9343C
#define INCLUDE_LANG_AST_HPP_4EAF8E6910F9343C

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "hbt/Lang/type.hpp"
#include "hbt/Util/bigint.hpp"

namespace hbt {
namespace lang {

class SourceLocation {
private:
	const std::string* _file;
	size_t _line, _col;
	const SourceLocation* _parent;

public:
	SourceLocation(const std::string* file, size_t line, size_t col, const SourceLocation* parent = nullptr)
		: _file(file), _line(line), _col(col), _parent(parent)
	{}

	const std::string& file() const { return *_file; }
	size_t line() const { return _line; }
	size_t col() const { return _col; }
	const SourceLocation* parent() const { return _parent; }
};



class Identifier {
private:
	SourceLocation _sloc;
	std::string _name;

public:
	Identifier(const SourceLocation& sloc, const std::string& name)
		: _sloc(sloc), _name(name)
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const std::string& name() const { return _name; }
};




class ASTVisitor;
class IdentifierExpr;
template<typename T>
class TypedLiteral;
class CastExpr;
class UnaryExpr;
class BinaryExpr;
class ConditionalExpr;
class EndpointExpr;
class CallExpr;
class AssignStmt;
class WriteStmt;
class IfStmt;
class SwitchStmt;
class BlockStmt;
class DeclarationStmt;
class GotoStmt;
class OnSimpleBlock;
class OnExprBlock;
class OnUpdateBlock;
class Endpoint;
class Device;
class MachineClass;
class MachineDefinition;
class MachineInstantiation;
class Behaviour;
class BehaviourClass;
class BehaviourDefinition;
class BehaviourInstantiation;
class IncludeLine;
class TranslationUnit;
class State;

class ASTVisitable {
public:
	virtual ~ASTVisitable() {}

	virtual void accept(ASTVisitor&) = 0;
};

struct ASTVisitor {
	virtual ~ASTVisitor() {}

	void visit(ASTVisitable& av)
	{
		av.accept(*this);
	}

	virtual void visit(IdentifierExpr&) = 0;
	virtual void visit(TypedLiteral<bool>&) = 0;
	virtual void visit(TypedLiteral<uint8_t>&) = 0;
	virtual void visit(TypedLiteral<uint16_t>&) = 0;
	virtual void visit(TypedLiteral<uint32_t>&) = 0;
	virtual void visit(TypedLiteral<uint64_t>&) = 0;
	virtual void visit(TypedLiteral<int8_t>&) = 0;
	virtual void visit(TypedLiteral<int16_t>&) = 0;
	virtual void visit(TypedLiteral<int32_t>&) = 0;
	virtual void visit(TypedLiteral<int64_t>&) = 0;
	virtual void visit(TypedLiteral<float>&) = 0;
	virtual void visit(CastExpr&) = 0;
	virtual void visit(UnaryExpr&) = 0;
	virtual void visit(BinaryExpr&) = 0;
	virtual void visit(ConditionalExpr&) = 0;
	virtual void visit(EndpointExpr&) = 0;
	virtual void visit(CallExpr&) = 0;

	virtual void visit(AssignStmt&) = 0;
	virtual void visit(WriteStmt&) = 0;
	virtual void visit(IfStmt&) = 0;
	virtual void visit(SwitchStmt&) = 0;
	virtual void visit(BlockStmt&) = 0;
	virtual void visit(DeclarationStmt&) = 0;
	virtual void visit(GotoStmt&) = 0;

	virtual void visit(OnSimpleBlock&) = 0;
	virtual void visit(OnExprBlock&) = 0;
	virtual void visit(OnUpdateBlock&) = 0;

	virtual void visit(Endpoint&) = 0;
	virtual void visit(Device&) = 0;
	virtual void visit(MachineClass&) = 0;
	virtual void visit(MachineDefinition&) = 0;
	virtual void visit(MachineInstantiation&) = 0;
	virtual void visit(BehaviourClass&) = 0;
	virtual void visit(BehaviourDefinition&) = 0;
	virtual void visit(BehaviourInstantiation&) = 0;
	virtual void visit(IncludeLine&) = 0;
	virtual void visit(TranslationUnit&) = 0;
};



class Declaration {
public:
	virtual ~Declaration() {}

	virtual const std::string& identifier() const = 0;
	virtual const SourceLocation& sloc() const = 0;
};



enum class EndpointAccess {
	Read          = 0x01,
	Write         = 0x02,
	NonLocalWrite = 0x04,
	Broadcast     = 0x08,
};

inline EndpointAccess operator&(EndpointAccess a, EndpointAccess b)
{ return static_cast<EndpointAccess>(static_cast<unsigned>(a) & static_cast<unsigned>(b)); }

inline EndpointAccess operator|(EndpointAccess a, EndpointAccess b)
{ return static_cast<EndpointAccess>(static_cast<unsigned>(a) | static_cast<unsigned>(b)); }

inline EndpointAccess operator^(EndpointAccess a, EndpointAccess b)
{ return static_cast<EndpointAccess>(static_cast<unsigned>(a) ^ static_cast<unsigned>(b)); }

inline EndpointAccess operator~(EndpointAccess a)
{ return static_cast<EndpointAccess>(~static_cast<unsigned>(a) & 0x0f); }

inline EndpointAccess operator&=(EndpointAccess& a, EndpointAccess b)
{ a = a & b; return a; }

inline EndpointAccess operator|=(EndpointAccess& a, EndpointAccess b)
{ a = a | b; return a; }



class Expr : public ASTVisitable {
private:
	SourceLocation _sloc;
	Type _type;
	bool _isConstexpr, _isIncomplete;
	util::Bigint _constexprValue;

protected:
	typedef std::unique_ptr<Expr> ptr_t;

	Expr(const SourceLocation& sloc, Type type, util::Bigint constexprValue = 0)
		: _sloc(sloc), _type(type), _isConstexpr(false), _isIncomplete(false), _constexprValue(constexprValue)
	{}

public:
	const SourceLocation& sloc() const { return _sloc; }
	Type type() const { return _type; }
	bool isConstexpr() const { return _isConstexpr; }
	bool isIncomplete() const { return _isIncomplete; }
	const util::Bigint& constexprValue() const { return _constexprValue; }

	void type(Type t) { _type = t; }
	void isConstexpr(bool b) { _isConstexpr = b; }
	void isIncomplete(bool b) { _isIncomplete = b; }
	void constexprValue(const util::Bigint& v) { _constexprValue = v; }
};

class IdentifierExpr : public Expr {
private:
	std::string _name;
	Declaration* _target;
	Expr* _value;

public:
	IdentifierExpr(const SourceLocation& sloc, std::string name, Type type)
		: Expr(sloc, type), _name(std::move(name)), _target(nullptr), _value(nullptr)
	{}

	const std::string& name() const { return _name; }
	Declaration* target() { return _target; }
	Expr* value() { return _value; }

	void target(Declaration* i) { _target = i; }
	void value(Expr* v) { _value = v; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class Literal : public Expr {
protected:
	using Expr::Expr;
};

template<typename T>
class TypedLiteral : public Literal {
	static constexpr Type calcType()
	{
		return
			std::is_same<T, bool>::value ? Type::Bool :
			std::is_same<T, uint8_t>::value ? Type::UInt8 :
			std::is_same<T, uint16_t>::value ? Type::UInt16 :
			std::is_same<T, uint32_t>::value ? Type::UInt32 :
			std::is_same<T, uint64_t>::value ? Type::UInt64 :
			std::is_same<T, int8_t>::value ? Type::Int8 :
			std::is_same<T, int16_t>::value ? Type::Int16 :
			std::is_same<T, int32_t>::value ? Type::Int32 :
			std::is_same<T, int64_t>::value ? Type::Int64 :
			Type::Float;
	}

	static_assert((calcType() == Type::Float) == std::is_same<T, float>::value, "bad literal type");

	typedef typename std::conditional<
			std::is_integral<T>::value && std::is_signed<T>::value,
			int64_t,
			uint64_t
		>::type widenened_const_type;

private:
	T _value;

public:
	TypedLiteral(const SourceLocation& sloc, T value)
		: Literal(sloc, calcType(), static_cast<widenened_const_type>(value)), _value(value)
	{
		isConstexpr(isConstexprType(calcType()));
	}

	T value() const { return _value; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class CastExpr : public Expr {
private:
	ptr_t _expr, _typeSource;

public:
	CastExpr(const SourceLocation& sloc, Type type, ptr_t expr)
		: Expr(sloc, type), _expr(std::move(expr))
	{}

	CastExpr(const SourceLocation& sloc, ptr_t typeSource, ptr_t expr)
		: Expr(sloc, Type::Int32), _expr(std::move(expr)), _typeSource(std::move(typeSource))
	{}

	Expr& expr() { return *_expr; }
	Expr* typeSource() { return _typeSource.get(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

enum class UnaryOperator {
	Plus,
	Minus,
	Not,
	Negate,
};

class UnaryExpr : public Expr {
private:
	UnaryOperator _op;
	ptr_t _expr;

public:
	UnaryExpr(const SourceLocation& sloc, UnaryOperator op, ptr_t expr)
		: Expr(sloc, Type::Int32), _op(op), _expr(std::move(expr))
	{}

	UnaryOperator op() const { return _op; }
	Expr& expr() { return *_expr; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

enum class BinaryOperator {
	Plus,
	Minus,
	Multiply,
	Divide,
	Modulo,
	And,
	Or,
	Xor,
	BoolAnd,
	BoolOr,
	Equals,
	NotEquals,
	LessThan,
	LessOrEqual,
	GreaterThan,
	GreaterOrEqual,
	ShiftLeft,
	ShiftRight,
};

class BinaryExpr : public Expr {
private:
	BinaryOperator _op;
	std::unique_ptr<Expr> _left, _right;

public:
	BinaryExpr(const SourceLocation& sloc, ptr_t left, BinaryOperator op, ptr_t right)
		: Expr(sloc, Type::Int32), _op(op), _left(std::move(left)), _right(std::move(right))
	{}

	BinaryOperator op() const { return _op; }
	Expr& left() { return *_left; }
	Expr& right() { return *_right; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class ConditionalExpr : public Expr {
private:
	ptr_t _cond, _true, _false;

public:
	ConditionalExpr(const SourceLocation& sloc, ptr_t cond, ptr_t ifTrue, ptr_t ifFalse)
		: Expr(sloc, Type::Int32), _cond(std::move(cond)), _true(std::move(ifTrue)), _false(std::move(ifFalse))
	{}

	Expr& condition() { return *_cond; }
	Expr& ifTrue() { return *_true; }
	Expr& ifFalse() { return *_false; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class EndpointExpr : public Expr {
private:
	std::vector<Identifier> _path;
	bool _deviceIdIsIncomplete, _endpointIdIsIncomplete;
	std::vector<Declaration*> _devices;
	Declaration* _behaviour;
	Declaration* _endpoint;

public:
	EndpointExpr(const SourceLocation& sloc, std::vector<Identifier> path, Type type)
		: Expr(sloc, type), _path(std::move(path)), _deviceIdIsIncomplete(false),
		  _endpointIdIsIncomplete(false), _behaviour(nullptr), _endpoint(nullptr)
	{}

	const std::vector<Identifier>& path() const { return _path; }
	bool deviceIdIsIncomplete() const { return _deviceIdIsIncomplete; }
	bool endpointIdIsIncomplete() const { return _endpointIdIsIncomplete; }
	const std::vector<Declaration*>& devices() { return _devices; }
	Declaration* behaviour() { return _behaviour; }
	Declaration* endpoint() { return _endpoint; }

	void devices(std::vector<Declaration*> d) { _devices = std::move(d); }
	void behaviour(Declaration* b) { _behaviour = b; }
	void endpoint(Declaration* e) { _endpoint = e; }

	void deviceIdIsIncomplete(bool b)
	{
		_deviceIdIsIncomplete = b;
		isIncomplete(_deviceIdIsIncomplete || _endpointIdIsIncomplete);
	}

	void endpointIdIsIncomplete(bool b)
	{
		_endpointIdIsIncomplete = b;
		isIncomplete(_deviceIdIsIncomplete || _endpointIdIsIncomplete);
	}

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class CallExpr : public Expr {
private:
	Identifier _name;
	std::vector<ptr_t> _arguments;
	Declaration* _target;

public:
	CallExpr(const SourceLocation& sloc, const Identifier& name, std::vector<ptr_t> arguments, Type type)
		: Expr(sloc, type), _name(name), _arguments(std::move(arguments)), _target(nullptr)
	{}

	const Identifier& name() const { return _name; }
	std::vector<ptr_t>& arguments() { return _arguments; }
	Declaration* target() { return _target; }

	void target(Declaration* d) { _target = d; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class Stmt : public ASTVisitable {
private:
	SourceLocation _sloc;

protected:
	typedef std::unique_ptr<Stmt> ptr_t;

	Stmt(const SourceLocation& sloc)
		: _sloc(sloc)
	{}

public:
	const SourceLocation& sloc() const { return _sloc; }
};

class AssignStmt : public Stmt {
private:
	Identifier _target;
	std::unique_ptr<Expr> _value;
	DeclarationStmt* _targetDecl;

public:
	AssignStmt(const SourceLocation& sloc, const Identifier& target, std::unique_ptr<Expr> value)
		: Stmt(sloc), _target(target), _value(std::move(value)), _targetDecl(nullptr)
	{}

	const Identifier& target() const { return _target; }
	Expr& value() { return *_value; }
	DeclarationStmt* targetDecl() { return _targetDecl; }

	void targetDecl(DeclarationStmt* d) { _targetDecl = d; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class WriteStmt : public Stmt {
private:
	EndpointExpr _target;
	std::unique_ptr<Expr> _value;

public:
	WriteStmt(const SourceLocation& sloc, EndpointExpr target, std::unique_ptr<Expr> value)
		: Stmt(sloc), _target(std::move(target)), _value(std::move(value))
	{}

	EndpointExpr& target() { return _target; }
	Expr& value() { return *_value; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class IfStmt : public Stmt {
private:
	std::unique_ptr<Expr> _condition;
	ptr_t _true, _false;

public:
	IfStmt(const SourceLocation& sloc, std::unique_ptr<Expr> condition, ptr_t ifTrue, ptr_t ifFalse)
		: Stmt(sloc), _condition(std::move(condition)), _true(std::move(ifTrue)), _false(std::move(ifFalse))
	{}

	Expr& condition() { return *_condition; }
	Stmt& ifTrue() { return *_true; }
	Stmt* ifFalse() { return _false.get(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class SwitchLabel {
private:
	SourceLocation _sloc;
	std::unique_ptr<Expr> _expr;

public:
	SwitchLabel(const SourceLocation& sloc, std::unique_ptr<Expr> expr)
		: _sloc(sloc), _expr(std::move(expr))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	Expr* expr() { return _expr.get(); }
};

class SwitchEntry {
private:
	std::vector<SwitchLabel> _labels;
	std::unique_ptr<Stmt> _stmt;

public:
	SwitchEntry(std::vector<SwitchLabel> labels, std::unique_ptr<Stmt> stmt)
		: _labels(std::move(labels)), _stmt(std::move(stmt))
	{}

	std::vector<SwitchLabel>& labels() { return _labels; }
	Stmt& statement() { return *_stmt; }
};

class SwitchStmt : public Stmt {
private:
	std::unique_ptr<Expr> _expr;
	std::vector<SwitchEntry> _entries;

public:
	SwitchStmt(const SourceLocation& sloc, std::unique_ptr<Expr> expr, std::vector<SwitchEntry> entries)
		: Stmt(sloc), _expr(std::move(expr)), _entries(std::move(entries))
	{}

	Expr& expr() { return *_expr; }
	std::vector<SwitchEntry>& entries() { return _entries; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class BlockStmt : public Stmt {
private:
	std::vector<ptr_t> _stmts;

public:
	BlockStmt(const SourceLocation& sloc, std::vector<ptr_t> stmts)
		: Stmt(sloc), _stmts(std::move(stmts))
	{}

	std::vector<ptr_t>& statements() { return _stmts; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class DeclarationStmt : public Stmt, public Declaration {
private:
	Type _type;
	Identifier _name;
	std::unique_ptr<Expr> _value, _typeSource;

public:
	DeclarationStmt(const SourceLocation& sloc, Type type, const Identifier& name, std::unique_ptr<Expr> value)
		: Stmt(sloc), _type(type), _name(name), _value(std::move(value))
	{}

	DeclarationStmt(const SourceLocation& sloc, std::unique_ptr<Expr> typeSource, const Identifier& name,
			std::unique_ptr<Expr> value)
		: Stmt(sloc), _type(Type::Int32), _name(name), _value(std::move(value)), _typeSource(std::move(typeSource))
	{}

	const SourceLocation& sloc() const override { return Stmt::sloc(); }
	Type type() const { return _type; }
	const Identifier& name() const { return _name; }
	Expr& value() { return *_value; }

	void type(Type t) { _type = t; }
	Expr* typeSource() { return _typeSource.get(); }

	const std::string& identifier() const override { return _name.name(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class GotoStmt : public Stmt {
private:
	Identifier _state;
	State* _target;

public:
	GotoStmt(const SourceLocation& sloc, const Identifier& state)
		: Stmt(sloc), _state(state)
	{}

	const Identifier& state() const { return _state; }
	State* target() const { return _target; }

	void target(State* s) { _target = s; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class OnBlock : public ASTVisitable {
private:
	SourceLocation _sloc;
	std::unique_ptr<BlockStmt> _block;

protected:
	OnBlock(const SourceLocation& sloc, std::unique_ptr<BlockStmt> block)
		: _sloc(sloc), _block(std::move(block))
	{}

public:
	const SourceLocation& sloc() const { return _sloc; }
	BlockStmt& block() { return *_block; }
};

enum class OnSimpleTrigger {
	Entry,
	Exit,
	Periodic,
	Always,
};

class OnSimpleBlock : public OnBlock {
private:
	OnSimpleTrigger _trigger;

public:
	OnSimpleBlock(const SourceLocation& sloc, OnSimpleTrigger trigger, std::unique_ptr<BlockStmt> block)
		: OnBlock(sloc, std::move(block)), _trigger(trigger)
	{}

	OnSimpleTrigger trigger() const { return _trigger; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class OnExprBlock : public OnBlock {
private:
	std::unique_ptr<Expr> _condition;

public:
	OnExprBlock(const SourceLocation& sloc, std::unique_ptr<Expr> condition, std::unique_ptr<BlockStmt> block)
		: OnBlock(sloc, std::move(block)), _condition(std::move(condition))
	{}

	Expr& condition() { return *_condition; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class OnUpdateBlock : public OnBlock {
private:
	EndpointExpr _endpoint;

public:
	OnUpdateBlock(const SourceLocation& sloc, EndpointExpr ep, std::unique_ptr<BlockStmt> block)
		: OnBlock(sloc, std::move(block)), _endpoint(std::move(ep))
	{}

	EndpointExpr& endpoint() { return _endpoint; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class State {
private:
	SourceLocation _sloc;
	Identifier _name;
	std::vector<std::unique_ptr<DeclarationStmt>> _variables;
	std::vector<std::unique_ptr<OnBlock>> _onBlocks;
	unsigned _id;
	bool _entered, _left;

public:
	State(const SourceLocation& sloc, const Identifier& name, std::vector<std::unique_ptr<DeclarationStmt>> variables,
			decltype(_onBlocks) onBlocks)
		: _sloc(sloc), _name(name), _variables(std::move(variables)), _onBlocks(std::move(onBlocks)), _id(0), _entered(false),
		  _left(false)
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	const std::vector<std::unique_ptr<DeclarationStmt>>& variables() const { return _variables; }
	std::vector<std::unique_ptr<OnBlock>>& onBlocks() { return _onBlocks; }
	unsigned id() const { return _id; }
	bool entered() const { return _entered; }
	bool left() const { return _left; }

	void id(unsigned u) { _id = u; }
	void entered(bool b) { _entered = b; }
	void left(bool b) { _left = b; }
};



class ProgramPart : public ASTVisitable {
private:
	SourceLocation _sloc;

public:
	ProgramPart(const SourceLocation& sloc)
		: _sloc(sloc)
	{}

	const SourceLocation& sloc() const { return _sloc; }
};



class EndpointDeclaration : public Declaration {
public:
	virtual Type type() const = 0;
	virtual const Identifier& name() const = 0;
};

class Endpoint : public ProgramPart, public EndpointDeclaration {
private:
	Identifier _name;
	uint32_t _eid;
	Type _type;
	EndpointAccess _access;

public:
	Endpoint(const SourceLocation& sloc, const Identifier& name, uint32_t eid, Type type, EndpointAccess access)
		: ProgramPart(sloc), _name(name), _eid(eid), _type(type), _access(access)
	{}

	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }
	const Identifier& name() const override { return _name; }
	uint32_t eid() const { return _eid; }
	Type type() const override { return _type; }
	EndpointAccess access() const { return _access; }

	const std::string& identifier() const override { return _name.name(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class SyntheticEndpoint : public EndpointDeclaration {
private:
	SourceLocation _sloc;
	Identifier _name;
	Type _type;
	std::unique_ptr<BlockStmt> _readBlock, _write;
	std::unique_ptr<Expr> _readValue;

public:
	SyntheticEndpoint(const SourceLocation& sloc, const Identifier& name, Type type, std::unique_ptr<BlockStmt> readBlock,
			std::unique_ptr<Expr> readValue, std::unique_ptr<BlockStmt> write)
		: _sloc(sloc), _name(name), _type(type), _readBlock(std::move(readBlock)), _write(std::move(write)),
		  _readValue(std::move(readValue))
	{}

	const Identifier& name() const override { return _name; }
	Type type() const override { return _type; }
	BlockStmt* readBlock() { return _readBlock.get(); }
	Expr* readValue() { return _readValue.get(); }
	BlockStmt* write() { return _write.get(); }

	const SourceLocation& sloc() const override { return _sloc; }
	const std::string& identifier() const override { return _name.name(); }
};



class Device : public ProgramPart, public Declaration {
private:
	Identifier _name;
	std::array<uint8_t, 16> _address;
	std::vector<Identifier> _endpointNames;
	std::map<std::string, Endpoint*> _endpoints;
	std::set<BehaviourDefinition*> _behaviours;
	std::multimap<BehaviourClass*, BehaviourDefinition*> _behaviourClasses;

public:
	Device(const SourceLocation& sloc, const Identifier& name, const std::array<uint8_t, 16>& address,
			std::vector<Identifier> endpointNames)
		: ProgramPart(sloc), _name(name), _address(address), _endpointNames(std::move(endpointNames))
	{}

	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }
	const Identifier& name() const { return _name; }
	const std::array<uint8_t, 16>& address() const { return _address; }
	const std::vector<Identifier>& endpointNames() const { return _endpointNames; }
	const std::set<BehaviourDefinition*>& behaviours() const { return _behaviours; }
	const std::multimap<BehaviourClass*, BehaviourDefinition*>& behaviourClasses() const { return _behaviourClasses; }

	const std::string& identifier() const override { return _name.name(); }

	std::map<std::string, Endpoint*>& endpoints() { return _endpoints; }
	std::set<BehaviourDefinition*>& behaviours() { return _behaviours; }
	std::multimap<BehaviourClass*, BehaviourDefinition*>& behaviourClasses() { return _behaviourClasses; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class ClassParameter : public Declaration {
public:
	enum class Kind {
		Value,
		Device,
		DeviceList,
		Endpoint,
	};

private:
	SourceLocation _sloc;
	std::string _name;
	Kind _kind;
	bool _used;

protected:
	ClassParameter(const SourceLocation& sloc, const std::string& name, Kind kind)
		: _sloc(sloc), _name(name), _kind(kind), _used(false)
	{}

public:
	virtual ~ClassParameter() {}

	const SourceLocation& sloc() const override { return _sloc; }
	const std::string& name() const { return _name; }
	const std::string& identifier() const override { return _name; }
	Kind kind() const { return _kind; }
	bool used() const { return _used; }

	void used(bool u) { _used = u; }
};

class CPValue : public ClassParameter {
private:
	Type _type;
	std::unique_ptr<Expr> _typeSource;

public:
	CPValue(const SourceLocation& sloc, const std::string& name, Type type)
		: ClassParameter(sloc, name, Kind::Value), _type(type)
	{}

	CPValue(const SourceLocation& sloc, const std::string& name, std::unique_ptr<Expr> typeSource)
		: ClassParameter(sloc, name, Kind::Value), _type(Type::Int32), _typeSource(std::move(typeSource))
	{}

	Type type() const { return _type; }
	void type(Type t) { _type = t; }
	Expr* typeSource() { return _typeSource.get(); }
};

class CPDevice : public ClassParameter {
public:
	CPDevice(const SourceLocation& sloc, const std::string& name)
		: ClassParameter(sloc, name, Kind::Device)
	{}
};

class CPDeviceList : public ClassParameter {
public:
	CPDeviceList(const SourceLocation& sloc, const std::string& name)
		: ClassParameter(sloc, name, Kind::DeviceList)
	{}
};

class CPEndpoint : public ClassParameter {
public:
	CPEndpoint(const SourceLocation& sloc, const std::string& name)
		: ClassParameter(sloc, name, Kind::Endpoint)
	{}
};


class ClassArgument {
public:
	enum class Kind {
		Expr,
		IdList,
		Device,
		DeviceList,
		Endpoint,
		SyntheticEndpoint,
	};

private:
	Kind _kind;
	SourceLocation _sloc;

protected:
	ClassArgument(Kind kind, SourceLocation sloc)
		: _kind(kind), _sloc(std::move(sloc))
	{}

public:
	Kind kind() const { return _kind; }
	const SourceLocation& sloc() const { return _sloc; }

	virtual ~ClassArgument() {}
};

class CAExpr : public ClassArgument {
private:
	std::unique_ptr<Expr> _expr;

public:
	CAExpr(std::unique_ptr<Expr> expr)
		: ClassArgument(Kind::Expr, expr->sloc()), _expr(std::move(expr))
	{}

	Expr& expr()
	{
		return *_expr;
	}
};

class CAIdList : public ClassArgument {
private:
	std::vector<Identifier> _ids;

public:
	CAIdList(SourceLocation sloc, std::vector<Identifier> ids)
		: ClassArgument(Kind::IdList, std::move(sloc)), _ids(std::move(ids))
	{}

	std::vector<Identifier>& ids()
	{
		return _ids;
	}
};

class CADevice : public ClassArgument {
private:
	Device* _device;

public:
	CADevice(SourceLocation sloc, Device* device)
		: ClassArgument(Kind::Device, std::move(sloc)), _device(device)
	{}

	Device& device()
	{
		return *_device;
	}
};

class CADeviceList : public ClassArgument {
private:
	std::vector<Device*> _devices;

public:
	CADeviceList(SourceLocation sloc, std::vector<Device*> devices)
		: ClassArgument(Kind::DeviceList, std::move(sloc)), _devices(std::move(devices))
	{}

	std::vector<Device*>& devices()
	{
		return _devices;
	}
};

class CAEndpoint : public ClassArgument {
private:
	Endpoint* _endpoint;

public:
	CAEndpoint(SourceLocation sloc, Endpoint* endpoint)
		: ClassArgument(Kind::Endpoint, std::move(sloc)), _endpoint(endpoint)
	{}

	Endpoint& endpoint()
	{
		return *_endpoint;
	}
};

class CASyntheticEndpoint : public ClassArgument {
private:
	Declaration* _behaviour;
	SyntheticEndpoint* _ep;

public:
	CASyntheticEndpoint(SourceLocation sloc, Declaration* behaviour, SyntheticEndpoint* ep)
		: ClassArgument(Kind::SyntheticEndpoint, std::move(sloc)), _behaviour(behaviour), _ep(ep)
	{}

	Declaration& behaviour()
	{
		return *_behaviour;
	}

	SyntheticEndpoint& endpoint()
	{
		return *_ep;
	}
};



class MachineBody {
private:
	Identifier _name;
	std::vector<std::unique_ptr<DeclarationStmt>> _variables;
	std::vector<State> _states;

protected:
	MachineBody(const Identifier& name, std::vector<std::unique_ptr<DeclarationStmt>> variables, std::vector<State> states)
		: _name(name), _variables(std::move(variables)), _states(std::move(states))
	{}

public:
	virtual ~MachineBody() {}

	const Identifier& name() const { return _name; }
	std::vector<std::unique_ptr<DeclarationStmt>>& variables() { return _variables; }
	std::vector<State>& states() { return _states; }
};

class MachineClass : public MachineBody, public ProgramPart, public Declaration {
private:
	std::vector<std::unique_ptr<ClassParameter>> _parameters;

public:
	MachineClass(const SourceLocation& sloc, const Identifier& name, std::vector<std::unique_ptr<ClassParameter>> parameters,
			std::vector<std::unique_ptr<DeclarationStmt>> variables, std::vector<State> states)
		: MachineBody(name, std::move(variables), std::move(states)),
		  ProgramPart(sloc), _parameters(std::move(parameters))
	{
	}

	std::vector<std::unique_ptr<ClassParameter>>& parameters() { return _parameters; }

	const std::string& identifier() const override { return name().name(); }
	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class MachineDefinition : public MachineBody, public ProgramPart, public Declaration {
public:
	MachineDefinition(const SourceLocation& sloc, const Identifier& name, std::vector<std::unique_ptr<DeclarationStmt>> variables,
			std::vector<State> states)
		: MachineBody(name, std::move(variables), std::move(states)), ProgramPart(sloc)
	{}

	const std::string& identifier() const override { return name().name(); }
	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class MachineInstantiation : public ProgramPart, public Declaration {
private:
	Identifier _name;
	Identifier _instanceOf;
	std::vector<std::unique_ptr<ClassArgument>> _arguments;

	MachineClass* _class;
	std::unique_ptr<MachineDefinition> _instantiation;

public:
	MachineInstantiation(const SourceLocation& sloc, const Identifier& name, const Identifier& instanceOf,
			std::vector<std::unique_ptr<ClassArgument>> arguments)
		: ProgramPart(sloc), _name(name), _instanceOf(instanceOf), _arguments(std::move(arguments)), _class(nullptr)
	{}

	const Identifier& name() const { return _name; }
	const Identifier& instanceOf() const { return _instanceOf; }
	std::vector<std::unique_ptr<ClassArgument>>& arguments() { return _arguments; }

	const std::string& identifier() const override { return name().name(); }
	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }

	MachineClass* baseClass() { return _class; }
	MachineDefinition* instantiation() { return _instantiation.get(); }

	void baseClass(MachineClass* m) { _class = m; }
	void instantiation(std::unique_ptr<MachineDefinition> u) { _instantiation = std::move(u); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class Behaviour {
private:
	Identifier _name;
	std::vector<std::unique_ptr<DeclarationStmt>> _variables;
	std::vector<std::unique_ptr<SyntheticEndpoint>> _endpoints;
	std::vector<std::unique_ptr<OnBlock>> _onBlocks;

protected:
	Behaviour(const Identifier& name, std::vector<std::unique_ptr<DeclarationStmt>> variables,
			std::vector<std::unique_ptr<SyntheticEndpoint>> endpoints, std::vector<std::unique_ptr<OnBlock>> onBlocks)
		: _name(name), _variables(std::move(variables)), _endpoints(std::move(endpoints)), _onBlocks(std::move(onBlocks))
	{}

public:
	Behaviour(Behaviour&&) = default;
	Behaviour& operator=(Behaviour&&) = default;

	virtual ~Behaviour() {}

	const Identifier& name() const { return _name; }
	const std::vector<std::unique_ptr<DeclarationStmt>>& variables() { return _variables; }
	const std::vector<std::unique_ptr<SyntheticEndpoint>>& endpoints() { return _endpoints; }
	const std::vector<std::unique_ptr<OnBlock>>& onBlocks() { return _onBlocks; }
};

class BehaviourClass : public Behaviour, public ProgramPart, public Declaration {
private:
	std::vector<std::unique_ptr<ClassParameter>> _parameters;

public:
	BehaviourClass(const SourceLocation& sloc, const Identifier& name, std::vector<std::unique_ptr<ClassParameter>> parameters,
			std::vector<std::unique_ptr<DeclarationStmt>> variables, std::vector<std::unique_ptr<SyntheticEndpoint>> endpoints,
			std::vector<std::unique_ptr<OnBlock>> onBlocks)
		: Behaviour(name, std::move(variables), std::move(endpoints), std::move(onBlocks)),
		  ProgramPart(sloc), _parameters(std::move(parameters))
	{}

	std::vector<std::unique_ptr<ClassParameter>>& parameters() { return _parameters; }

	const std::string& identifier() const override { return name().name(); }
	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class BehaviourDefinition : public Behaviour, public ProgramPart, public Declaration {
private:
	Identifier _device;

public:
	BehaviourDefinition(const SourceLocation& sloc, const Identifier& name, const Identifier& device,
			std::vector<std::unique_ptr<DeclarationStmt>> variables, std::vector<std::unique_ptr<SyntheticEndpoint>> endpoints,
			std::vector<std::unique_ptr<OnBlock>> onBlocks)
		: Behaviour(name, std::move(variables), std::move(endpoints), std::move(onBlocks)),
		  ProgramPart(sloc), _device(device)
	{}

	const Identifier& device() const { return _device; }

	const std::string& identifier() const override { return name().name(); }
	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class BehaviourInstantiation : public ProgramPart, public Declaration {
private:
	Identifier _name;
	Identifier _device;
	Identifier _instanceOf;
	std::vector<std::unique_ptr<ClassArgument>> _arguments;

	BehaviourClass* _class;
	std::unique_ptr<BehaviourDefinition> _instantiation;

public:
	BehaviourInstantiation(const SourceLocation& sloc, const Identifier& name, const Identifier& device,
			const Identifier& instanceOf, std::vector<std::unique_ptr<ClassArgument>> arguments)
		: ProgramPart(sloc), _name(name), _device(device), _instanceOf(instanceOf),
		  _arguments(std::move(arguments)), _class(nullptr)
	{}

	const Identifier& name() const { return _name; }
	const Identifier& device() const { return _device; }
	const Identifier& instanceOf() const { return _instanceOf; }
	std::vector<std::unique_ptr<ClassArgument>>& arguments() { return _arguments; }

	const std::string& identifier() const override { return name().name(); }
	const SourceLocation& sloc() const override { return ProgramPart::sloc(); }

	BehaviourClass* baseClass() { return _class; }
	BehaviourDefinition* instantiation() { return _instantiation.get(); }

	void baseClass(BehaviourClass* b) { _class = b; }
	void instantiation(std::unique_ptr<BehaviourDefinition> u) { _instantiation = std::move(u); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class IncludeLine : public ProgramPart {
private:
	std::string _file;
	std::string _fullPath;

public:
	IncludeLine(const SourceLocation& sloc, std::string file)
		: ProgramPart(sloc), _file(file)
	{}

	const std::string& file() const { return _file; }

	const std::string& fullPath() const { return _fullPath; }
	void fullPath(std::string path) { _fullPath = std::move(path); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class TranslationUnit : public ASTVisitable {
private:
	std::unique_ptr<std::string> _file;
	std::vector<std::unique_ptr<ProgramPart>> _items;

public:
	TranslationUnit(std::unique_ptr<std::string> file, std::vector<std::unique_ptr<ProgramPart>> items)
		: _file(std::move(file)), _items(std::move(items))
	{}

	const std::string& file() const { return *_file; }
	const std::vector<std::unique_ptr<ProgramPart>>& items() const { return _items; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

}
}

#endif
