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
	Identifier(const SourceLocation& sloc, const std::string& name)
		: _sloc(sloc), _name(name)
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const std::string& name() const { return _name; }
};




class ASTVisitor;
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

class ASTVisitable {
public:
	virtual ~ASTVisitable() {}

	virtual void accept(ASTVisitor&) = 0;
};

struct ASTVisitor {
	virtual ~ASTVisitor();

	void visit(ASTVisitable& av)
	{
		av.accept(*this);
	}

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

protected:
	typedef std::unique_ptr<Expr> ptr_t;

	Expr(const SourceLocation& sloc, Type type)
		: _sloc(sloc), _type(type)
	{}

public:
	const SourceLocation& sloc() const { return _sloc; }
	Type type() const { return _type; }

	void type(Type t) { _type = t; }
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

	const Expr& expr() const { return *_expr; }

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

	static Type unaryType(UnaryOperator op, Type type)
	{
		if (type == Type::Unknown)
			return type;

		return op == UnaryOperator::Not
			? Type::Bool
			: type;
	}

public:
	UnaryExpr(const SourceLocation& sloc, UnaryOperator op, ptr_t&& expr)
		: Expr(sloc, unaryType(op, expr->type())), _op(op), _expr(std::move(expr))
	{}

	UnaryOperator op() const { return _op; }
	const Expr& expr() const { return *_expr; }

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
		: Expr(sloc, commonType(left->type(), right->type())), _op(op),
		  _left(std::move(left)), _right(std::move(right))
	{}

	BinaryOperator op() const { return _op; }
	const Expr& left() const { return *_left; }
	const Expr& right() const { return *_right; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class ConditionalExpr : public Expr {
private:
	ptr_t _cond, _true, _false;

	static Type ternaryType(Type a, Type b)
	{
		return a == b ? a : commonType(a, b);
	}

public:
	ConditionalExpr(const SourceLocation& sloc, ptr_t&& cond, ptr_t&& ifTrue, ptr_t&& ifFalse)
		: Expr(sloc, ternaryType(ifTrue->type(), ifFalse->type())), _cond(std::move(cond)),
		  _true(std::move(ifTrue)), _false(std::move(ifFalse))
	{}

	const Expr& condition() const { return *_cond; }
	const Expr& ifTrue() const { return *_true; }
	const Expr& ifFalse() const { return *_false; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class EndpointExpr : public Expr {
private:
	Identifier _device;
	Identifier _endpoint;

public:
	EndpointExpr(const SourceLocation& sloc, const Identifier& device, const Identifier& endpoint, Type type)
		: Expr(sloc, type), _device(device), _endpoint(endpoint)
	{}

	const Identifier& device() const { return _device; }
	const Identifier& endpoint() const { return _endpoint; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class CallExpr : public Expr {
private:
	std::string _name;
	std::vector<ptr_t> _arguments;

public:
	CallExpr(const SourceLocation& sloc, const std::string& name, std::vector<ptr_t>&& arguments, Type type)
		: Expr(sloc, type), _name(name), _arguments(std::move(arguments))
	{}

	const std::string& name() const { return _name; }
	const std::vector<ptr_t>& arguments() const { return _arguments; }

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
	const Expr& value() const { return *_value; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class WriteStmt : public Stmt {
private:
	Identifier _device;
	Identifier _endpoint;
	std::unique_ptr<Expr> _value;

public:
	WriteStmt(const SourceLocation& sloc, const Identifier& device, const Identifier& endpoint, std::unique_ptr<Expr>&& value)
		: Stmt(sloc), _device(device), _endpoint(endpoint), _value(std::move(value))
	{}

	const Identifier& device() const { return _device; }
	const Identifier& endpoint() const { return _endpoint; }
	const Expr& value() const { return *_value; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class IfStmt : public Stmt {
private:
	std::unique_ptr<Expr> _selector;
	ptr_t _true, _false;

public:
	IfStmt(const SourceLocation& sloc, std::unique_ptr<Expr>&& sel, ptr_t&& ifTrue, ptr_t&& ifFalse)
		: Stmt(sloc), _selector(std::move(sel)), _true(std::move(ifTrue)), _false(std::move(ifFalse))
	{}

	const Expr& selector() const { return *_selector; }
	const Stmt& ifTrue() const { return *_true; }
	const Stmt* ifFalse() const { return _false.get(); }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
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
	std::unique_ptr<Expr> _expr;
	std::vector<SwitchEntry> _entries;

public:
	SwitchStmt(const SourceLocation& sloc, std::unique_ptr<Expr>&& expr, std::vector<SwitchEntry>&& entries)
		: Stmt(sloc), _expr(std::move(expr)), _entries(std::move(entries))
	{}

	const Expr& expr() const { return *_expr; }
	const std::vector<SwitchEntry>& entries() const { return _entries; }

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

	const std::vector<ptr_t>& statements() const { return _stmts; }

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
	const Expr* value() const { return _value.get(); }

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
	const BlockStmt& block() const { return *_block; }
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

	const Expr& condition() const { return *_condition; }

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
	const std::vector<DeclarationStmt>& variables() const { return _variables; }
	const std::vector<std::unique_ptr<OnBlock>>& onBlocks() const { return _onBlocks; }
	const std::vector<std::unique_ptr<Stmt>>& statements() const { return _statements; }
};



class ProgramPart : public ASTVisitable {
};



class Endpoint : public ProgramPart {
private:
	SourceLocation _sloc;
	Identifier _name;
	uint32_t _eid;
	Type _type;
	EndpointAccess _access;

public:
	Endpoint(const SourceLocation& sloc, const Identifier& name, uint32_t eid, Type type, EndpointAccess access)
		: _sloc(sloc), _name(name), _eid(eid), _type(type), _access(access)
	{}

	const SourceLocation& sloc() const { return _sloc; }
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
	SourceLocation _sloc;
	Identifier _name;
	std::array<uint8_t, 16> _address;
	std::vector<Identifier> _endpoints;

public:
	Device(const SourceLocation& sloc, const Identifier& name, const std::array<uint8_t, 16>& address,
			std::vector<Identifier>&& endpoints)
		: _sloc(sloc), _name(name), _address(address), _endpoints(std::move(endpoints))
	{}

	const SourceLocation& sloc() const { return _sloc; }
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
	SourceLocation _sloc;
	Identifier _name;
	std::vector<DeclarationStmt> _variables;
	std::vector<State> _states;

protected:
	MachineBody(const SourceLocation& sloc, const Identifier& name,
			std::vector<DeclarationStmt>&& variables, std::vector<State>&& states)
		: _sloc(sloc), _name(name), _variables(std::move(variables)), _states(std::move(states))
	{}

public:
	virtual ~MachineBody() {}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	const std::vector<DeclarationStmt>& variables() const { return _variables; }
	const std::vector<State>& states() const { return _states; }
};

class MachineClass : public MachineBody, public ProgramPart {
private:
	std::vector<Identifier> _parameters;

public:
	MachineClass(const SourceLocation& sloc, const Identifier& name, std::vector<Identifier>&& parameters,
			std::vector<DeclarationStmt>&& variables, std::vector<State>&& states)
		: MachineBody(sloc, name, std::move(variables), std::move(states)),
		  _parameters(std::move(parameters))
	{
	}

	const std::vector<Identifier>& parameters() const { return _parameters; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class MachineDefinition : public MachineBody, public ProgramPart {
public:
	MachineDefinition(const SourceLocation& sloc, const Identifier& name, std::vector<DeclarationStmt>&& variables,
			std::vector<State>&& states)
		: MachineBody(sloc, name, std::move(variables), std::move(states))
	{}

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};

class MachineInstantiation : public ProgramPart {
private:
	SourceLocation _sloc;
	Identifier _name;
	Identifier _instanceOf;
	std::vector<std::unique_ptr<Expr>> _arguments;

public:
	MachineInstantiation(const SourceLocation& sloc, const Identifier& name, const Identifier& instanceOf,
			std::vector<std::unique_ptr<Expr>>&& arguments)
		: _sloc(sloc), _name(name), _instanceOf(instanceOf), _arguments(std::move(arguments))
	{}

	const SourceLocation& sloc() const { return _sloc; }
	const Identifier& name() const { return _name; }
	const Identifier& instanceOf() const { return _instanceOf; }
	const std::vector<std::unique_ptr<Expr>>& arguments() const { return _arguments; }

	virtual void accept(ASTVisitor& v)
	{
		v.visit(*this);
	}
};



class IncludeLine : public ProgramPart {
private:
	SourceLocation _sloc;
	std::string _file;
	std::string _fullPath;

public:
	IncludeLine(const SourceLocation& sloc, std::string&& file)
		: _sloc(sloc), _file(file)
	{}

	const SourceLocation& sloc() const { return _sloc; }
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
