#ifndef INCLUDE_IR_INSTRUCTION_HPP_BCDA16AC543B8A1A
#define INCLUDE_IR_INSTRUCTION_HPP_BCDA16AC543B8A1A

#include <array>
#include <exception>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/optional.hpp>

namespace hbt {
namespace ir {

enum class Opcode {
	LD_SOURCE_IP,
	LD_SOURCE_EID,
	LD_SOURCE_VAL,
	LD_CURSTATE,
	LD_CURSTATETIME,
	LD_FALSE,
	LD_TRUE,
	LD_U8,
	LD_U16,
	LD_U32,
	LD_FLOAT,
	LD_DT,
	LD_SYSTIME,

	LD_REG,
	ST_REG,

	MUL,
	DIV,
	MOD,
	ADD,
	SUB,
	DT_DIFF,
	AND,
	OR,
	XOR,
	NOT,
	SHL,
	SHR,
	DUP,
	DUP_I,
	ROT,
	ROT_I,
	DT_DECOMPOSE,
	GETTYPE,
	SWITCH_8,
	SWITCH_16,
	SWITCH_32,

	CMP_BLOCK,
	CMP_IP_LO,
	CMP_LT,
	CMP_LE,
	CMP_GT,
	CMP_GE,
	CMP_EQ,
	CMP_NEQ,

	CONV_B,
	CONV_U8,
	CONV_U32,
	CONV_F,

	JNZ,
	JZ,
	JUMP,

	WRITE,

	POP,

	RET_CHANGE,
	RET_STAY,
};



enum class DTMask {
	second  = 1,
	minute  = 2,
	hour    = 4,
	day     = 8,
	month   = 16,
	year    = 32,
	weekday = 64,
};

inline DTMask operator&(DTMask a, DTMask b)
{ return static_cast<DTMask>(static_cast<unsigned>(a) & static_cast<unsigned>(b)); }

inline DTMask operator|(DTMask a, DTMask b)
{ return static_cast<DTMask>(static_cast<unsigned>(a) | static_cast<unsigned>(b)); }

inline DTMask operator^(DTMask a, DTMask b)
{ return static_cast<DTMask>(static_cast<unsigned>(a) ^ static_cast<unsigned>(b)); }

inline DTMask operator~(DTMask a)
{ return static_cast<DTMask>(~static_cast<unsigned>(a) & 0x7f); }

inline DTMask operator&=(DTMask& a, DTMask b)
{ a = a & b; return a; }

inline DTMask operator|=(DTMask& a, DTMask b)
{ a = a | b; return a; }

inline bool invalid(DTMask m)
{ return m != ~~m; }



class DateTime {
	private:
		uint8_t _second;
		uint8_t _minute;
		uint8_t _hour;
		uint8_t _day;
		uint8_t _month;
		uint16_t _year;
		uint8_t _weekday;

	public:
		DateTime(uint8_t second, uint8_t minute, uint8_t hour,
				uint8_t day, uint8_t month, uint16_t year,
				uint8_t weekday)
			: _second(second), _minute(minute), _hour(hour), _day(day),
			  _month(month), _year(year), _weekday(weekday)
		{}

		uint8_t second() const { return _second; }
		uint8_t minute() const { return _minute; }
		uint8_t hour() const { return _hour; }
		uint8_t day() const { return _day; }
		uint8_t month() const { return _month; }
		uint16_t year() const { return _year; }
		uint8_t weekday() const { return _weekday; }
};



class Label {
	private:
		size_t _id;
		std::string _name;

	public:
		explicit Label(size_t id, const std::string& name = "")
			: _id(id), _name(name)
		{}

		size_t id() const { return _id; }
		const std::string& name() const { return _name; }
};



struct SwitchEntry {
	uint32_t label;
	Label target;
};

class SwitchTable {
	private:
		std::vector<SwitchEntry> _entries;

	public:
		typedef decltype(_entries.cbegin()) iterator_t;

	public:
		template<typename Iter>
		SwitchTable(Iter begin, Iter end)
			: _entries(begin, end)
		{
			if (end - begin > 255)
				throw std::invalid_argument("end");
		}

		size_t size() const { return _entries.size(); }

		iterator_t begin() const { return _entries.begin(); }
		iterator_t end() const { return _entries.end(); }
};



class BlockPart {
	public:
		typedef std::array<uint8_t, 16> block_t;

	private:
		unsigned _start, _length;
		block_t _block;

	public:
		BlockPart(unsigned start, unsigned length, const block_t& block)
			: _start(start), _length(length), _block(block)
		{
			if (start > 15)
				throw std::invalid_argument("start");
			if (length > 16 || start + length > 16)
				throw std::invalid_argument("length");
		}

		unsigned start() const { return _start; }
		unsigned length() const { return _length; }
		const block_t& block() const { return _block; }
};



class Instruction {
	private:
		Opcode _opcode;
		boost::optional<Label> _label;
		unsigned _line;

	public:
		Instruction(Opcode opcode, boost::optional<Label> label = {}, unsigned line = 0)
			: _opcode(opcode), _label(label), _line(line)
		{}

		virtual ~Instruction();

		Opcode opcode() const { return _opcode; }
		const Label* label() const { return _label.get_ptr(); }
		unsigned line() const { return _line; }
};

template<typename Immed>
class ImmediateInstruction : public Instruction {
	static_assert(
		std::is_same<Immed, SwitchTable>::value ||
		std::is_same<Immed, BlockPart>::value ||
		std::is_same<Immed, uint8_t>::value ||
		std::is_same<Immed, uint16_t>::value ||
		std::is_same<Immed, uint32_t>::value ||
		std::is_same<Immed, float>::value ||
		std::is_same<Immed, DTMask>::value ||
		std::is_same<Immed, Label>::value ||
		std::is_same<Immed, std::tuple<DTMask, DateTime>>::value
		, "");
	private:
		Immed _immed;

	public:
		ImmediateInstruction(Opcode opcode, const Immed& immed,
				boost::optional<Label> label = {}, unsigned line = 0)
			: Instruction(opcode, label, line), _immed(immed)
		{}

		const Immed& immed() const { return _immed; }
};

class InvalidInstruction : public Instruction {
	private:
		std::string _comment;
	public:
		InvalidInstruction(Opcode opcode, const std::string& comment, unsigned line)
			: Instruction(opcode, {}, line), _comment(comment)
		{}

		const std::string& comment() const { return _comment; }
};

}
}

#endif
