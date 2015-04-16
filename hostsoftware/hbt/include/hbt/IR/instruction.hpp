#ifndef INSTRUCTION__HPP_5A8A7DB7E0D74693
#define INSTRUCTION__HPP_5A8A7DB7E0D74693

#include <array>
#include <map>
#include <string>
#include <utility>

#include "hbt/Util/bigint.hpp"

namespace hbt {
namespace ir {

class BasicBlock;



enum class Type {
	Bool,
	UInt8,
	UInt16,
	UInt32,
	UInt64,
	Int8,
	Int16,
	Int32,
	Int64,
	Float,
};



class Value {
private:
	Type _type;
	std::string _name;

public:
	Value(Type type, std::string name)
		: _type(type), _name(std::move(name))
	{}

	virtual ~Value();

	Type type() const { return _type; }
	const std::string& name() const { return _name; }
};



enum class InsnType {
	Arithmetic,
	Cast,
	SpecialValue,
	IntegralConstant,
	FloatConstant,
	Load,
	Store,
	Switch,
	Jump,
	Return,
	CompareIP,
	Write,
	ExtractDatePart,
	Phi,
};

class Instruction {
private:
	InsnType _type;

public:
	Instruction(InsnType type)
		: _type(type)
	{}

	InsnType insnType() const { return _type; }
	bool isTerminator() const
	{
		return _type == InsnType::Switch || _type == InsnType::Jump || _type == InsnType::Return;
	}

	virtual ~Instruction();
};

enum class ArithOp {
	Mul,
	Div,
	Mod,
	Add,
	Sub,
	And,
	Or,
	Xor,
	Shl,
	Shr,
	Lt,
	Le,
	Gt,
	Ge,
	Eq,
	Neq,
};

class ArithmeticInsn : public Instruction, public Value {
private:
	ArithOp _op;
	const Value* _left;
	const Value* _right;

public:
	ArithmeticInsn(std::string name, Type type, ArithOp op, const Value* left, const Value* right)
		: Instruction(InsnType::Arithmetic), Value(type, std::move(name)), _op(op), _left(left), _right(right)
	{}

	ArithOp op() const { return _op; }
	const Value* left() const { return _left; }
	const Value* right() const { return _right; }
};

class UnaryInsn : public Instruction {
private:
	const Value* _value;

protected:
	UnaryInsn(InsnType type, const Value* value)
		: Instruction(type), _value(value)
	{}

public:
	const Value* value() const { return _value; }
};

class CastInsn : public UnaryInsn, public Value {
public:
	CastInsn(std::string name, Type type, const Value* value)
		: UnaryInsn(InsnType::Cast, value), Value(type, std::move(name))
	{}
};

enum class SpecialVal {
	SourceEID,
	SourceVal,
	SysTime,
};

class LoadSpecialInsn : public Instruction, public Value {
private:
	SpecialVal _val;

public:
	LoadSpecialInsn(std::string name, Type type, SpecialVal val)
		: Instruction(InsnType::SpecialValue), Value(type, std::move(name)), _val(val)
	{}

	SpecialVal val() const { return _val; }
};

class LoadIntInsn : public Instruction, public Value {
private:
	util::Bigint _value;

public:
	LoadIntInsn(std::string name, Type type, util::Bigint val)
		: Instruction(InsnType::IntegralConstant), Value(type, std::move(name)), _value(val)
	{}

	const util::Bigint& value() const { return _value; }
};

class LoadFloatInsn : public Instruction, public Value {
private:
	float _value;

public:
	LoadFloatInsn(std::string name, float val)
		: Instruction(InsnType::FloatConstant), Value(Type::Float, std::move(name)), _value(val)
	{}

	float value() const { return _value; }
};

class LoadInsn : public Instruction, public Value {
private:
	unsigned _address;

public:
	LoadInsn(std::string name, Type type, unsigned addr)
		: Instruction(InsnType::Load), Value(type, std::move(name)), _address(addr)
	{}

	unsigned address() const { return _address; }
};

class StoreInsn : public UnaryInsn {
private:
	unsigned _address;

public:
	StoreInsn(unsigned addr, const Value* val)
		: UnaryInsn(InsnType::Store, val), _address(addr)
	{}

	unsigned address() const { return _address; }
};

class SwitchInsn : public UnaryInsn {
public:
	typedef std::map<util::Bigint, const BasicBlock*> labels_type;
private:
	labels_type _labels;
	const BasicBlock* _defaultLabel;

public:
	SwitchInsn(const Value* val, labels_type labels, const BasicBlock* defaultLabel)
		: UnaryInsn(InsnType::Switch, val), _labels(std::move(labels)), _defaultLabel(defaultLabel)
	{}

	const labels_type& labels() const { return _labels; }
	const BasicBlock* defaultLabel() const { return _defaultLabel; }
};

class JumpInsn : public Instruction {
private:
	const BasicBlock* _target;

public:
	JumpInsn(const BasicBlock* target)
		: Instruction(InsnType::Jump), _target(target)
	{}

	const BasicBlock* target() const { return _target; }
};

class ReturnInsn : public Instruction {
public:
	ReturnInsn()
		: Instruction(InsnType::Return)
	{}
};

class CompareIPInsn : public Instruction, public Value {
private:
	unsigned _start, _length;
	std::array<uint8_t, 16> _block;

public:
	CompareIPInsn(std::string name, unsigned start, unsigned len, std::array<uint8_t, 16> block)
		: Instruction(InsnType::CompareIP), Value(Type::Bool, std::move(name)), _start(start), _length(len), _block(block)
	{}

	unsigned start() const { return _start; }
	unsigned length() const { return _length; }
	const std::array<uint8_t, 16>& block() const { return _block; }
};

class WriteInsn : public UnaryInsn {
private:
	uint32_t _eid;

public:
	WriteInsn(uint32_t eid, const Value* value)
		: UnaryInsn(InsnType::Write, value), _eid(eid)
	{}

	uint32_t eid() const { return _eid; }
};

enum class DatePart {
	Second,
	Minute,
	Hour,
	Day,
	Month,
	Year,
	Weekday,
};

class ExtractDatePartInsn : public UnaryInsn, public Value {
private:
	DatePart _part;

public:
	ExtractDatePartInsn(std::string name, DatePart part, const Value* value)
		: UnaryInsn(InsnType::ExtractDatePart, value), Value(Type::Int32, std::move(name)), _part(part)
	{}

	DatePart part() const { return _part; }
};

class PhiInsn : public Instruction, public Value {
public:
	typedef std::map<const BasicBlock*, const Value*> sources_type;
private:
	sources_type _sources;

public:
	PhiInsn(std::string name, Type type, sources_type sources)
		: Instruction(InsnType::Phi), Value(type, std::move(name)), _sources(std::move(sources))
	{}

	const sources_type sources() const { return _sources; }
};

}
}

#endif
