#ifndef INCLUDE_LANG_AST_HPP_4EAF8E6910F9343C
#define INCLUDE_LANG_AST_HPP_4EAF8E6910F9343C

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "Lang/type.hpp"

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
class PacketEIDExpr;
class TimeoutExpr;
class AssignStmt;
class WriteStmt;
class IfStmt;
class SwitchStmt;
class BlockStmt;
class DeclarationStmt;
class GotoStmt;
class OnSimpleBlock;
class OnPacketBlock;
class OnExprBlock;
class Endpoint;
class Device;
class MachineClass;
class MachineDefinition;
class MachineInstantiation;
class IncludeLine;
class TranslationUnit;

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
	virtual void visit(TypedLiteral<uint32_t>&) = 0;
	virtual void visit(TypedLiteral<uint64_t>&) = 0;
	virtual void visit(TypedLiteral<float>&) = 0;
	virtual void visit(CastExpr&) = 0;
	virtual void visit(UnaryExpr&) = 0;
	virtual void visit(BinaryExpr&) = 0;
	virtual void visit(ConditionalExpr&) = 0;
	virtual void visit(EndpointExpr&) = 0;
	virtual void visit(CallExpr&) = 0;
	virtual void visit(PacketEIDExpr&) = 0;
	virtual void visit(TimeoutExpr&) = 0;

	virtual void visit(AssignStmt&) = 0;
	virtual void visit(WriteStmt&) = 0;
	virtual void visit(IfStmt&) = 0;
	virtual void visit(SwitchStmt&) = 0;
	virtual void visit(BlockStmt&) = 0;
	virtual void visit(DeclarationStmt&) = 0;
	virtual void visit(GotoStmt&) = 0;

	virtual void visit(OnSimpleBlock&) = 0;
	virtual void visit(OnPacketBlock&) = 0;
	virtual void visit(OnExprBlock&) = 0;

	virtual void visit(Endpoint&) = 0;
	virtual void visit(Device&) = 0;
	virtual void visit(MachineClass&) = 0;
	virtual void visit(MachineDefinition&) = 0;
	virtual void visit(MachineInstantiation&) = 0;
	virtual void visit(IncludeLine&) = 0;
	virtual void visit(TranslationUnit&) = 0;
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
	bool _isConstant, _isDependent;

protected:
	typedef std::unique_ptr<Expr> ptr_t;

	Expr(const SourceLocation& sloc, Type type)
		: _sloc(sloc), _type(type), _isConstant(false), _isDependent(false)
	{}

public:
	const SourceLocation& sloc() const { return _sloc; }
	Type type() const { return _type; }
	bool isConstant() const { return _isConstant; }
	bool isDependent() const { return _isDependent; }

	void type(Type t) { _type = t; }
	void isConstant(bool b) { _isConstant = b; }
	void isDependent(bool b) { _isDependent = b; }
};

class IdentifierExpr : public Expr {
private:
	std::string _name;

public:
	IdentifierExpr(const SourceLocation& sloc, std::string&& name, Type type)
		: Expr(sloc, type), _name(std::move(name))
	{}

	const std::string& name() const { return _name; }

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
			std::is_same<T, uint32_t>::value ? Type::UInt32 :
			std::is_same<T, uint64_t>::value ? Type::UInt64 :
			std::is_same<T, float>::value ? Type::Float :
			Type::Unknown;
	}

	static_assert(calcType() != Type::Unknown, "bad literal type");

private:
	T _value;

public:
	TypedLiteral(const SourceLocation& sloc, T value)
		: Literal(sloc, calcType()), _value(value)
	{}

	T value() const { return _value; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class CastExpr : public Expr {
private:
	ptr_t _expr;

public:
	CastExpr(const SourceLocation& sloc, Type type, ptr_t&& expr)
		: Expr(sloc, type), _expr(std::move(expr))
	{}

	Expr& expr() { return *_expr; }

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
	UnaryExpr(const SourceLocation& sloc, UnaryOperator op, ptr_t&& expr)
		: Expr(sloc, Type::Unknown), _op(op), _expr(std::move(expr))
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
	BinaryExpr(const SourceLocation& sloc, ptr_t&& left, BinaryOperator op, ptr_t&& right)
		: Expr(sloc, Type::Unknown), _op(op), _left(std::move(left)), _right(std::move(right))
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
	ConditionalExpr(const SourceLocation& sloc, ptr_t&& cond, ptr_t&& ifTrue, ptr_t&& ifFalse)
		: Expr(sloc, Type::Unknown), _cond(std::move(cond)), _true(std::move(ifTrue)), _false(std::move(ifFalse))
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
	Identifier _device;
	Identifier _endpoint;
	bool _deviceIsDependent, _endpointIsDependent;

public:
	EndpointExpr(const SourceLocation& sloc, const Identifier& device, const Identifier& endpoint, Type type)
		: Expr(sloc, type), _device(device), _endpoint(endpoint), _deviceIsDependent(false), _endpointIsDependent(false)
	{}

	const Identifier& device() const { return _device; }
	const Identifier& endpoint() const { return _endpoint; }
	bool deviceIsDependent() const { return _deviceIsDependent; }
	bool endpointIsDependent() const { return _endpointIsDependent; }

	void deviceIsDependent(bool b) {  _deviceIsDependent = b; isDependent(_deviceIsDependent || _endpointIsDependent); }
	void endpointIsDependent(bool b) {  _endpointIsDependent = b; isDependent(_deviceIsDependent || _endpointIsDependent); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class CallExpr : public Expr {
private:
	Identifier _name;
	std::vector<ptr_t> _arguments;

public:
	CallExpr(const SourceLocation& sloc, const Identifier& name, std::vector<ptr_t>&& arguments, Type type)
		: Expr(sloc, type), _name(name), _arguments(std::move(arguments))
	{}

	const Identifier& name() const { return _name; }
	std::vector<ptr_t>& arguments() { return _arguments; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class PacketEIDExpr : public Expr {
public:
	PacketEIDExpr(const SourceLocation& sloc)
		: Expr(sloc, Type::UInt32)
	{}

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class TimeoutExpr : public Expr {
public:
	TimeoutExpr(const SourceLocation& sloc)
		: Expr(sloc, Type::UInt32)
	{}

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

public:
	AssignStmt(const SourceLocation& sloc, const Identifier& target, std::unique_ptr<Expr>&& value)
		: Stmt(sloc), _target(target), _value(std::move(value))
	{}

	const Identifier& target() const { return _target; }
	Expr& value() { return *_value; }

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
	WriteStmt(const SourceLocation& sloc, EndpointExpr target, std::unique_ptr<Expr>&& value)
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
	IfStmt(const SourceLocation& sloc, std::unique_ptr<Expr>&& condition, ptr_t&& ifTrue, ptr_t&& ifFalse)
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
	SwitchEntry(std::vector<SwitchLabel> labels, std::unique_ptr<Stmt>&& stmt)
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
	SwitchStmt(const SourceLocation& sloc, std::unique_ptr<Expr>&& expr, std::vector<SwitchEntry>&& entries)
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
	BlockStmt(const SourceLocation& sloc, std::vector<ptr_t>&& stmts)
		: Stmt(sloc), _stmts(std::move(stmts))
	{}

	std::vector<ptr_t>& statements() { return _stmts; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class DeclarationStmt : public Stmt {
private:
	Type _type;
	Identifier _name;
	std::unique_ptr<Expr> _value;

public:
	DeclarationStmt(const SourceLocation& sloc, Type type, const Identifier& name, std::unique_ptr<Expr>&& value = nullptr)
		: Stmt(sloc), _type(type), _name(name), _value(std::move(value))
	{}

	Type type() const { return _type; }
	const Identifier& name() const { return _name; }
	Expr* value() { return _value.get(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class GotoStmt : public Stmt {
private:
	Identifier _state;

public:
	GotoStmt(const SourceLocation& sloc, const Identifier& state)
		: Stmt(sloc), _state(state)
	{}

	const Identifier& state() const { return _state; }

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
	OnBlock(const SourceLocation& sloc, std::unique_ptr<BlockStmt>&& block)
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
};

class OnSimpleBlock : public OnBlock {
private:
	OnSimpleTrigger _trigger;

public:
	OnSimpleBlock(const SourceLocation& sloc, OnSimpleTrigger trigger, std::unique_ptr<BlockStmt>&& block)
		: OnBlock(sloc, std::move(block)), _trigger(trigger)
	{}

	OnSimpleTrigger trigger() const { return _trigger; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class OnPacketBlock : public OnBlock {
private:
	Identifier _source;

public:
	OnPacketBlock(const SourceLocation& sloc, const Identifier& source, std::unique_ptr<BlockStmt>&& block)
		: OnBlock(sloc, std::move(block)), _source(source)
	{}

	const Identifier& source() const { return _source; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class OnExprBlock : public OnBlock {
private:
	std::unique_ptr<Expr> _condition;

public:
	OnExprBlock(const SourceLocation& sloc, std::unique_ptr<Expr>&& condition, std::unique_ptr<BlockStmt>&& block)
		: OnBlock(sloc, std::move(block)), _condition(std::move(condition))
	{}

	Expr& condition() { return *_condition; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class State {
private:
	SourceLocation _sloc;
	Identifier _name;
	std::vector<DeclarationStmt> _variables;
	std::vector<std::unique_ptr<OnBlock>> _onBlocks;
	std::vector<std::unique_ptr<Stmt>> _statements;

public:
	State(const SourceLocation& sloc, const Identifier& name, std::vector<DeclarationStmt>&& variables,
			decltype(_onBlocks)&& onBlocks, decltype(_statements)&& statements)
		: _sloc(sloc), _name(name), _variables(std::move(variables)),
		  _onBlocks(std::move(onBlocks)), _statements(std::move(statements))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	std::vector<DeclarationStmt>& variables() { return _variables; }
	std::vector<std::unique_ptr<OnBlock>>& onBlocks() { return _onBlocks; }
	std::vector<std::unique_ptr<Stmt>>& statements() { return _statements; }
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



class Endpoint : public ProgramPart {
private:
	Identifier _name;
	uint32_t _eid;
	Type _type;
	EndpointAccess _access;

public:
	Endpoint(const SourceLocation& sloc, const Identifier& name, uint32_t eid, Type type, EndpointAccess access)
		: ProgramPart(sloc), _name(name), _eid(eid), _type(type), _access(access)
	{}

	const Identifier& name() const { return _name; }
	uint32_t eid() const { return _eid; }
	Type type() const { return _type; }
	EndpointAccess access() const { return _access; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class Device : public ProgramPart {
private:
	Identifier _name;
	std::array<uint8_t, 16> _address;
	std::vector<Identifier> _endpoints;

public:
	Device(const SourceLocation& sloc, const Identifier& name, const std::array<uint8_t, 16>& address,
			std::vector<Identifier>&& endpoints)
		: ProgramPart(sloc), _name(name), _address(address), _endpoints(std::move(endpoints))
	{}

	const Identifier& name() const { return _name; }
	const std::array<uint8_t, 16>& address() const { return _address; }
	const std::vector<Identifier>& endpoints() const { return _endpoints; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class MachineBody {
private:
	Identifier _name;
	std::vector<DeclarationStmt> _variables;
	std::vector<State> _states;

protected:
	MachineBody(const Identifier& name, std::vector<DeclarationStmt>&& variables, std::vector<State>&& states)
		: _name(name), _variables(std::move(variables)), _states(std::move(states))
	{}

public:
	virtual ~MachineBody() {}

	const Identifier& name() const { return _name; }
	std::vector<DeclarationStmt>& variables() { return _variables; }
	std::vector<State>& states() { return _states; }
};

class ClassParameter {
public:
	enum class Type {
		Unknown,

		Value,
		Device,
		Endpoint
	};

private:
	SourceLocation _sloc;
	std::string _name;
	Type _type;

public:
	ClassParameter(const SourceLocation& sloc, const std::string& name, Type type = Type::Unknown)
		: _sloc(sloc), _name(name), _type(type)
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const std::string& name() const { return _name; }
	const Type type() const { return _type; }

	void type(Type type) { _type = type; }
};

class MachineClass : public MachineBody, public ProgramPart {
private:
	std::vector<ClassParameter> _parameters;

public:
	MachineClass(const SourceLocation& sloc, const Identifier& name, std::vector<ClassParameter>&& parameters,
			std::vector<DeclarationStmt>&& variables, std::vector<State>&& states)
		: MachineBody(name, std::move(variables), std::move(states)),
		  ProgramPart(sloc), _parameters(std::move(parameters))
	{
	}

	std::vector<ClassParameter>& parameters() { return _parameters; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class MachineDefinition : public MachineBody, public ProgramPart {
public:
	MachineDefinition(const SourceLocation& sloc, const Identifier& name, std::vector<DeclarationStmt>&& variables,
			std::vector<State>&& states)
		: MachineBody(name, std::move(variables), std::move(states)), ProgramPart(sloc)
	{}

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class MachineInstantiation : public ProgramPart {
private:
	Identifier _name;
	Identifier _instanceOf;
	std::vector<std::unique_ptr<Expr>> _arguments;

public:
	MachineInstantiation(const SourceLocation& sloc, const Identifier& name, const Identifier& instanceOf,
			std::vector<std::unique_ptr<Expr>>&& arguments)
		: ProgramPart(sloc), _name(name), _instanceOf(instanceOf), _arguments(std::move(arguments))
	{}

	const Identifier& name() const { return _name; }
	const Identifier& instanceOf() const { return _instanceOf; }
	std::vector<std::unique_ptr<Expr>>& arguments() { return _arguments; }

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
	IncludeLine(const SourceLocation& sloc, std::string&& file)
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
	TranslationUnit(std::unique_ptr<std::string>&& file, std::vector<std::unique_ptr<ProgramPart>>&& items)
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
