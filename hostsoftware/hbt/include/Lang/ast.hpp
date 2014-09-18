#ifndef INCLUDE_LANG_AST_HPP_4EAF8E6910F9343C
#define INCLUDE_LANG_AST_HPP_4EAF8E6910F9343C

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace hbt {
namespace lang {

class SourceLocation {
private:
	std::shared_ptr<std::string> _file;
	size_t _line, _col;
	const SourceLocation* _parent;

public:
	SourceLocation(const std::shared_ptr<std::string>& file, size_t line, size_t col, const SourceLocation* parent = nullptr)
		: _file(std::move(file)), _line(line), _col(col), _parent(parent)
	{}

	const std::string& file() const { return *_file; }
	size_t line() const { return _line; }
	size_t col() const { return _col; }
	const SourceLocation* parent() const { return _parent; }
};



enum class Type {
	Unknown,

	Bool,
	UInt8,
	UInt32,
	UInt64,
	Float,
};

inline Type commonType(Type a, Type b)
{
	if (a == Type::Unknown || b == Type::Unknown)
		return Type::Unknown;

	switch (a) {
	case Type::Unknown:
		return a;

	case Type::Bool:
	case Type::UInt8:
	case Type::UInt32:
		if (b == Type::UInt64)
			return Type::UInt64;
		else if (b == Type::Float)
			return Type::Float;
		else
			return Type::UInt32;

	case Type::UInt64:
		if (b == Type::Float)
			return Type::Float;
		else
			return Type::UInt64;

	case Type::Float:
		return Type::Float;
	}
}



class Identifier {
private:
	SourceLocation _sloc;
	std::string _name;

public:
	Identifier(SourceLocation&& sloc, std::string&& name)
		: _sloc(std::move(sloc)), _name(std::move(name))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const std::string& name() const { return _name; }
};



class ProgramPart {
public:
	virtual ~ProgramPart() {}
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

class Endpoint : public ProgramPart {
private:
	SourceLocation _sloc;
	Identifier _name;
	uint32_t _eid;
	Type _type;
	EndpointAccess _access;

public:
	Endpoint(SourceLocation&& sloc, Identifier&& name, uint32_t eid, Type type, EndpointAccess access)
		: _sloc(std::move(sloc)), _name(std::move(name)), _eid(eid), _type(type), _access(access)
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	uint32_t eid() const { return _eid; }
	Type type() const { return _type; }
	EndpointAccess access() const { return _access; }
};



class Device : public ProgramPart {
private:
	SourceLocation _sloc;
	Identifier _name;
	std::array<uint8_t, 16> _address;
	std::vector<const Endpoint*> _endpoints;

public:
	Device(SourceLocation&& sloc, Identifier&& name, const std::array<uint8_t, 16>& address,
			std::vector<const Endpoint*>&& endpoints)
		: _sloc(std::move(sloc)), _name(std::move(name)), _address(address), _endpoints(std::move(endpoints))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	const std::array<uint8_t, 16>& address() const { return _address; }
	const std::vector<const Endpoint*>& endpoints() const { return _endpoints; }

	bool hasEndpoint(const Endpoint& ep) const
	{
		return std::count(_endpoints.begin(), _endpoints.end(), &ep);
	}
};



class Expr {
private:
	SourceLocation _sloc;
	Type _type;

protected:
	typedef std::unique_ptr<Expr> ptr_t;

	Expr(SourceLocation&& sloc, Type type)
		: _sloc(sloc), _type(type)
	{}

public:
	virtual ~Expr() {}

	const SourceLocation& sloc() const { return _sloc; }
	Type type() const { return _type; }
};

class IdentifierExpr : public Expr {
private:
	std::string _name;

public:
	IdentifierExpr(SourceLocation&& sloc, std::string&& name, Type type)
		: Expr(std::move(sloc), type), _name(std::move(name))
	{}

	const std::string& name() const { return _name; }
};

class Literal : public Expr {
protected:
	using Expr::Expr;
};

template<typename T>
class TypedLiteral : public Literal {
	static_assert(
		std::is_same<T, bool>::value ||
		std::is_same<T, uint8_t>::value ||
		std::is_same<T, uint32_t>::value ||
		std::is_same<T, uint64_t>::value ||
		std::is_same<T, float>::value,
		"bad literal type");

	static Type calcType()
	{
		return
			std::is_same<T, bool>::value ? Type::Bool :
			std::is_same<T, uint8_t>::value ? Type::UInt8 :
			std::is_same<T, uint32_t>::value ? Type::UInt32 :
			std::is_same<T, uint64_t>::value ? Type::UInt64 :
			std::is_same<T, float>::value ? Type::Float :
			throw "invalid type";
	}

private:
	T _value;

public:
	TypedLiteral(SourceLocation&& sloc, T value)
		: Literal(std::move(sloc), calcType()), _value(value)
	{}

	T value() const { return _value; }
};

class CastExpr : public Expr {
private:
	ptr_t _expr;

public:
	CastExpr(SourceLocation&& sloc, Type type, ptr_t&& expr)
		: Expr(std::move(sloc), type), _expr(std::move(expr))
	{}

	const Expr& expr() const { return *_expr; }
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

	static Type unaryType(UnaryOperator op, Type type)
	{
		if (type == Type::Unknown)
			return type;

		return op == UnaryOperator::Not
			? Type::Bool
			: type;
	}

public:
	UnaryExpr(SourceLocation&& sloc, UnaryOperator op, ptr_t&& expr)
		: Expr(std::move(sloc), unaryType(op, expr->type())), _op(op), _expr(std::move(expr))
	{}

	UnaryOperator op() const { return _op; }
	const Expr& expr() const { return *_expr; }
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
};

class BinaryExpr : public Expr {
private:
	BinaryOperator _op;
	std::unique_ptr<Expr> _left, _right;

public:
	BinaryExpr(SourceLocation&& sloc, ptr_t&& left, BinaryOperator op, ptr_t&& right)
		: Expr(std::move(sloc), commonType(left->type(), right->type())), _op(op),
		  _left(std::move(left)), _right(std::move(right))
	{}

	BinaryOperator op() const { return _op; }
	const Expr& left() const { return *_left; }
	const Expr& right() const { return *_right; }
};

class ConditionalExpr : public Expr {
private:
	ptr_t _cond, _true, _false;

	static Type ternaryType(Type a, Type b)
	{
		return a == b ? a : commonType(a, b);
	}

public:
	ConditionalExpr(SourceLocation&& sloc, ptr_t&& cond, ptr_t&& ifTrue, ptr_t&& ifFalse)
		: Expr(std::move(sloc), ternaryType(ifTrue->type(), ifFalse->type())), _cond(std::move(cond)),
		  _true(std::move(ifTrue)), _false(std::move(ifFalse))
	{}

	const Expr& condition() const { return *_cond; }
	const Expr& ifTrue() const { return *_true; }
	const Expr& ifFalse() const { return *_false; }
};

class EndpointExpr : public Expr {
private:
	const Device& _device;
	const Endpoint& _endpoint;

public:
	EndpointExpr(SourceLocation&& sloc, const Device& device, const Endpoint& endpoint)
		: Expr(std::move(sloc), endpoint.type()), _device(device), _endpoint(endpoint)
	{}

	const Device& device() const { return _device; }
	const Endpoint& endpoint() const { return _endpoint; }
};

class CallExpr : public Expr {
private:
	std::string _name;
	std::vector<ptr_t> _arguments;

public:
	CallExpr(SourceLocation&& sloc, std::string&& name, std::vector<ptr_t>&& arguments, Type type)
		: Expr(std::move(sloc), type), _name(std::move(name)), _arguments(std::move(arguments))
	{}

	const std::string& name() const { return _name; }
	const std::vector<ptr_t>& arguments() const { return _arguments; }
};

enum class SystemProperty {
	Time,
	StateTime,
	PacketEID,
};

class SystemPropertyExpr : public Expr {
private:
	SystemProperty _prop;

	static Type propType(SystemProperty prop)
	{
		switch (prop) {
		case SystemProperty::Time: return Type::UInt64;
		case SystemProperty::StateTime: return Type::UInt32;
		case SystemProperty::PacketEID: return Type::UInt32;
		}
	}

public:
	SystemPropertyExpr(SourceLocation&& sloc, SystemProperty prop)
		: Expr(std::move(sloc), propType(prop)), _prop(prop)
	{}

	SystemProperty property() const { return _prop; }
};

class PacketValueExpr : public Expr {
private:
	const Endpoint& _endpoint;

public:
	PacketValueExpr(SourceLocation&& sloc, const Endpoint& endpoint)
		: Expr(std::move(sloc), endpoint.type()), _endpoint(endpoint)
	{}

	const Endpoint& endpoint() const { return _endpoint; }
};



class Stmt {
private:
	SourceLocation _sloc;

protected:
	typedef std::unique_ptr<Stmt> ptr_t;

	Stmt(SourceLocation&& sloc)
		: _sloc(std::move(sloc))
	{}

public:
	virtual ~Stmt() {}

	const SourceLocation& sloc() const { return _sloc; }
};

class AssignStmt : public Stmt {
private:
	Identifier _target;
	std::unique_ptr<Expr> _value;

public:
	AssignStmt(SourceLocation&& sloc, Identifier&& target, std::unique_ptr<Expr>&& value)
		: Stmt(std::move(sloc)), _target(std::move(target)), _value(std::move(value))
	{}

	const Identifier& target() const { return _target; }
	const Expr& value() const { return *_value; }
};

class WriteStmt : public Stmt {
private:
	const Device& _device;
	const Endpoint& _endpoint;
	std::unique_ptr<Expr> _value;

public:
	WriteStmt(SourceLocation&& sloc, const Device& device, const Endpoint& endpoint, std::unique_ptr<Expr>&& value)
		: Stmt(std::move(sloc)), _device(device), _endpoint(endpoint), _value(std::move(value))
	{}

	const Device& device() const { return _device; }
	const Endpoint& endpoint() const { return _endpoint; }
	const Expr& value() const { return *_value; }
};

class IfStmt : public Stmt {
private:
	std::unique_ptr<Expr> _selector;
	ptr_t _true, _false;

public:
	IfStmt(SourceLocation&& sloc, ptr_t&& ifTrue, ptr_t&& ifFalse)
		: Stmt(std::move(sloc)), _true(std::move(ifTrue)), _false(std::move(ifFalse))
	{}

	const Expr& selector() const { return *_selector; }
	const Stmt& ifTrue() const { return *_true; }
	const Stmt& isFalse() const { return *_false; }
};

class SwitchEntry {
private:
	std::vector<std::unique_ptr<Expr>> _labels;
	std::unique_ptr<Stmt> _stmt;

public:
	SwitchEntry(std::vector<std::unique_ptr<Expr>>&& labels, std::unique_ptr<Stmt>&& stmt)
		: _labels(std::move(labels)), _stmt(std::move(stmt))
	{}

	const std::vector<std::unique_ptr<Expr>>& labels() const { return _labels; }
	const Stmt& statement() const { return *_stmt; }
};

class SwitchStmt : public Stmt {
private:
	std::vector<SwitchEntry> _labels;
	ptr_t _default;

public:
	SwitchStmt(SourceLocation&& sloc, std::vector<SwitchEntry>&& labels, ptr_t defaultLabel = ptr_t())
		: Stmt(std::move(sloc)), _labels(std::move(labels)), _default(std::move(defaultLabel))
	{}

	const std::vector<SwitchEntry>& labels() const { return _labels; }
	const Stmt* defaultLabel() const { return _default.get(); }
};

class BlockStmt : public Stmt {
private:
	std::vector<ptr_t> _stmts;

public:
	BlockStmt(SourceLocation&& sloc, std::vector<ptr_t>&& stmts)
		: Stmt(std::move(sloc)), _stmts(std::move(stmts))
	{}

	const std::vector<ptr_t>& statements() const { return _stmts; }
};

class DeclarationStmt : public Stmt {
private:
	Type _type;
	Identifier _name;
	std::unique_ptr<Expr> _value;

public:
	DeclarationStmt(SourceLocation&& sloc, Type type, Identifier&& name, std::unique_ptr<Expr>&& value = nullptr)
		: Stmt(std::move(sloc)), _type(type), _name(std::move(name)), _value(std::move(value))
	{}

	Type type() const { return _type; }
	const Identifier& name() const { return _name; }
	const Expr* value() const { return _value.get(); }
};

class GotoStmt : public Stmt {
private:
	Identifier _state;

public:
	GotoStmt(SourceLocation&& sloc, Identifier&& state)
		: Stmt(std::move(sloc)), _state(std::move(state))
	{}

	const Identifier& state() const { return _state; }
};



enum class OnBlockTrigger {
	Entry,
	Exit,
	Packet,
	Periodic,
	Expr,
};

class OnBlock {
private:
	SourceLocation _sloc;
	OnBlockTrigger _trigger;

public:
	OnBlock(SourceLocation&& sloc, OnBlockTrigger trigger)
		: _sloc(sloc), _trigger(trigger)
	{}

	virtual ~OnBlock() {}

	const SourceLocation& sloc() const { return _sloc; }
	OnBlockTrigger trigger() const { return _trigger; }
};

class OnPacketBlock : public OnBlock {
private:
	std::array<uint8_t, 16> _source;

public:
	OnPacketBlock(SourceLocation&& sloc, const std::array<uint8_t, 16>& source)
		: OnBlock(std::move(sloc), OnBlockTrigger::Packet), _source(source)
	{}

	const std::array<uint8_t, 16> source() const { return _source; }
};

class OnExprBlock : public OnBlock {
private:
	std::unique_ptr<Expr> _condition;

public:
	OnExprBlock(SourceLocation&& sloc, std::unique_ptr<Expr>&& condition)
		: OnBlock(std::move(sloc), OnBlockTrigger::Packet), _condition(std::move(condition))
	{}

	const Expr& condition() const { return *_condition; }
};



class State {
private:
	SourceLocation _sloc;
	Identifier _name;
	std::vector<DeclarationStmt> _variables;
	std::vector<std::unique_ptr<OnBlock>> _onBlocks;
	std::vector<std::unique_ptr<Stmt>> _statements;

public:
	State(SourceLocation&& sloc, Identifier&& name, std::vector<DeclarationStmt>&& variables,
			decltype(_onBlocks)&& onBlocks, decltype(_statements)&& statements)
		: _sloc(std::move(sloc)), _name(std::move(name)), _variables(std::move(variables)),
		  _onBlocks(std::move(onBlocks)), _statements(std::move(statements))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	const std::vector<DeclarationStmt>& variables() const { return _variables; }
	const std::vector<std::unique_ptr<OnBlock>>& onBlocks() const { return _onBlocks; }
	const std::vector<std::unique_ptr<Stmt>>& statements() const { return _statements; }
};



class MachineBody {
private:
	std::vector<DeclarationStmt> _variables;
	std::vector<State> _states;

protected:
	MachineBody(std::vector<DeclarationStmt>&& variables, std::vector<State>&& states)
		: _variables(std::move(variables)), _states(std::move(states))
	{}

public:
	const std::vector<DeclarationStmt>& variables() const { return _variables; }
	const std::vector<State>& states() const { return _states; }
};

class MachineClass : public ProgramPart {
private:
	SourceLocation _sloc;
	Identifier _name;
	std::vector<Identifier> _parameters;
	MachineBody _body;

public:
	MachineClass(SourceLocation&& sloc, Identifier&& name, std::vector<Identifier>&& parameters,
			MachineBody&& body)
		: _sloc(std::move(sloc)), _name(std::move(name)), _parameters(std::move(_parameters)),
		  _body(std::move(body))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	const std::vector<Identifier>& parameters() const { return _parameters; }
	const MachineBody& body() const { return _body; }
};

class MachineDefinition : public ProgramPart {
private:
	SourceLocation _sloc;
	Identifier _name;
	MachineBody _body;

public:
	MachineDefinition(SourceLocation&& sloc, Identifier&& name, MachineBody&& body)
		: _sloc(std::move(sloc)), _name(std::move(name)), _body(std::move(body))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	const MachineBody& body() const { return _body; }
};

class MachineInstantiation : public ProgramPart {
private:
	SourceLocation _sloc;
	const MachineClass* _instanceOf;
	std::vector<std::unique_ptr<Expr>> _arguments;

public:
	MachineInstantiation(SourceLocation&& sloc, const MachineClass* instanceOf, std::vector<std::unique_ptr<Expr>>&& arguments)
		: _sloc(std::move(sloc)), _instanceOf(instanceOf), _arguments(std::move(arguments))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const MachineClass* instanceOf() const { return _instanceOf; }
	const std::vector<std::unique_ptr<Expr>>& arguments() const { return _arguments; }
};

}
}

#endif
