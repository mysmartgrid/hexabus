#ifndef INCLUDE_MC_INSTRUCTION_HPP_BCDA16AC543B8A1A
#define INCLUDE_MC_INSTRUCTION_HPP_BCDA16AC543B8A1A

#include <array>
#include <exception>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/optional.hpp>

namespace hbt {
namespace mc {

enum class Opcode {
	LD_SOURCE_EID,
	LD_SOURCE_VAL,
	LD_FALSE,
	LD_TRUE,
	LD_U8,
	LD_U16,
	LD_U32,
	LD_U64,
	LD_S8,
	LD_S16,
	LD_S32,
	LD_S64,
	LD_FLOAT,
	LD_SYSTIME,

	LD_MEM,
	ST_MEM,

	MUL,
	DIV,
	MOD,
	ADD,
	SUB,
	AND,
	OR,
	XOR,
	SHL,
	SHR,
	DUP,
	DUP_I,
	ROT,
	ROT_I,
	DT_DECOMPOSE,
	SWITCH_8,
	SWITCH_16,
	SWITCH_32,

	CMP_SRC_IP,
	CMP_LT,
	CMP_LE,
	CMP_GT,
	CMP_GE,
	CMP_EQ,
	CMP_NEQ,

	CONV_B,
	CONV_U8,
	CONV_U16,
	CONV_U32,
	CONV_U64,
	CONV_S8,
	CONV_S16,
	CONV_S32,
	CONV_S64,
	CONV_F,

	JNZ,
	JZ,
	JUMP,

	WRITE,

	POP,
	POP_I,

	RET,
};



enum class MemType {
	Bool,
	U8,
	U16,
	U32,
	U64,
	S8,
	S16,
	S32,
	S64,
	Float,
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
		std::is_same<Immed, uint64_t>::value ||
		std::is_same<Immed, int8_t>::value ||
		std::is_same<Immed, int16_t>::value ||
		std::is_same<Immed, int32_t>::value ||
		std::is_same<Immed, int64_t>::value ||
		std::is_same<Immed, float>::value ||
		std::is_same<Immed, DTMask>::value ||
		std::is_same<Immed, Label>::value ||
		std::is_same<Immed, std::tuple<MemType, uint16_t>>::value
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

}
}

#endif
