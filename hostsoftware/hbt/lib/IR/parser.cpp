#include "IR/parser.hpp"

#include "IR/builder.hpp"

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/tuple/tuple.hpp>

#include <algorithm>
#include <map>
#include <tuple>

#include <iostream>
#include <boost/format.hpp>

namespace {

struct datetime_immediate {
	unsigned second, minute, hour;
	unsigned day, month, year;
	unsigned weekday;

	operator hbt::ir::DateTime() const
	{
		return hbt::ir::DateTime(second, minute, hour, day, month, year, weekday);
	}
};

struct switch_entry {
	uint32_t label;
	std::string target;
};

struct block_immediate {
	unsigned start;
	std::vector<uint8_t> block;
};

struct ir_instruction {
	typedef boost::variant<
			uint8_t,
			uint32_t,
			float,
			std::vector<switch_entry>,
			block_immediate,
			hbt::ir::DTMask,
			std::string,
			datetime_immediate,
			std::tuple<hbt::ir::DTMask, datetime_immediate>
		> param_t;

	typedef boost::optional<param_t> immed_t;

	hbt::ir::Opcode opcode;
	immed_t immediate;
};

typedef boost::variant<std::string, ir_instruction> ir_line;

struct ir_program_header {
	std::vector<uint8_t> machine_id;
	std::string on_packet, on_periodic;
};

struct ir_program {
	ir_program_header header;

	std::vector<ir_line> lines;
};

}

BOOST_FUSION_ADAPT_STRUCT(
	datetime_immediate,
	(unsigned, second)
	(unsigned, minute)
	(unsigned, hour)
	(unsigned, day)
	(unsigned, month)
	(unsigned, year)
	(unsigned, weekday)
)

BOOST_FUSION_ADAPT_STRUCT(
	switch_entry,
	(uint32_t, label)
	(std::string, target)
)

BOOST_FUSION_ADAPT_STRUCT(
	block_immediate,
	(unsigned, start)
	(std::vector<uint8_t>, block)
)

BOOST_FUSION_ADAPT_STRUCT(
	ir_instruction,
	(hbt::ir::Opcode, opcode)
	(ir_instruction::immed_t, immediate)
)

BOOST_FUSION_ADAPT_STRUCT(
	ir_program_header,
	(std::vector<uint8_t>, machine_id)
	(std::string, on_packet)
	(std::string, on_periodic)
)

BOOST_FUSION_ADAPT_STRUCT(
	ir_program,
	(ir_program_header, header)
	(std::vector<ir_line>, lines)
)

namespace {

namespace qi = boost::spirit::qi;

template<typename It>
struct asm_ws : qi::grammar<It> {
	asm_ws() : asm_ws::base_type(start)
	{
		using namespace qi;

		start =
			(ascii::space - eol)
			| (lit(";") >> *(char_ - eol));
	}

	qi::rule<It> start;
};

struct simple_instructions : qi::symbols<char, ir_instruction> {
	simple_instructions()
	{
		using hbt::ir::Opcode;
		add
			("mul", { Opcode::MUL, boost::none_t() })
			("div", { Opcode::DIV, boost::none_t() })
			("mod", { Opcode::MOD, boost::none_t() })
			("add", { Opcode::ADD, boost::none_t() })
			("sub", { Opcode::SUB, boost::none_t() })
			("and", { Opcode::AND, boost::none_t() })
			("or", { Opcode::OR, boost::none_t() })
			("xor", { Opcode::XOR, boost::none_t() })
			("not", { Opcode::NOT, boost::none_t() })
			("shl", { Opcode::SHL, boost::none_t() })
			("shr", { Opcode::SHR, boost::none_t() })
			("gettype", { Opcode::GETTYPE, boost::none_t() })
			("dt.diff", { Opcode::DT_DIFF, boost::none_t() })
			("cmp.localhost", { Opcode::CMP_IP_LO, boost::none_t() })
			("cmp.lt", { Opcode::CMP_LT, boost::none_t() })
			("cmp.le", { Opcode::CMP_LE, boost::none_t() })
			("cmp.gt", { Opcode::CMP_GT, boost::none_t() })
			("cmp.ge", { Opcode::CMP_GE, boost::none_t() })
			("cmp.eq", { Opcode::CMP_EQ, boost::none_t() })
			("cmp.neq", { Opcode::CMP_NEQ, boost::none_t() })
			("conv.b", { Opcode::CONV_B, boost::none_t() })
			("conv.u8", { Opcode::CONV_U8, boost::none_t() })
			("conv.u32", { Opcode::CONV_U32, boost::none_t() })
			("conv.f", { Opcode::CONV_F, boost::none_t() })
			("write", { Opcode::WRITE, boost::none_t() })
			("pop", { Opcode::POP, boost::none_t() })
			("ret.change", { Opcode::RET_CHANGE, boost::none_t() })
			("ret.stay", { Opcode::RET_STAY, boost::none_t() });
	}
};

struct ld_simple_operands : qi::symbols<char, ir_instruction> {
	ld_simple_operands()
	{
		using hbt::ir::Opcode;
		add
			("src.ip", { Opcode::LD_SOURCE_IP, boost::none_t() })
			("src.eid", { Opcode::LD_SOURCE_EID, boost::none_t() })
			("src.val", { Opcode::LD_SOURCE_VAL, boost::none_t() })
			("sys.state", { Opcode::LD_CURSTATE, boost::none_t() })
			("sys.statetime", { Opcode::LD_CURSTATETIME, boost::none_t() })
			("false", { Opcode::LD_FALSE, boost::none_t() })
			("true", { Opcode::LD_TRUE, boost::none_t() })
			("sys.time", { Opcode::LD_SYSTIME, boost::none_t() });
	}
};

template<typename It>
struct as_grammar : qi::grammar<It, ir_program(), asm_ws<It>> {
	as_grammar()
		: as_grammar::base_type(start, "HBT IR program")
	{
		using namespace qi;
		using boost::phoenix::bind;
		using boost::phoenix::val;

		start %=
			machine_header
			> *(
				(label > eol)
				| (instruction || eol)
				| (!eoi > errors.label_or_instruction)
			)
			> eoi;

		machine_header.name("machine header");
		machine_header %=
			(lit(".version") > lit("0") > eol)
			> machine_id
			> on_packet_vector
			> on_periodic_vector;

		machine_id.name("machine identifier");
		machine_id =
			(
				lit(".machine")
				> lexeme[
					lit("0x")
					> repeat(1, 32)[uint_parser<uint8_t, 16, 1, 1>()]
				]
				> (eol | (eps > errors.machine_id_too_long))
			)[_val = bind(make_block, _1)];

		on_packet_vector.name("packet vector");
		on_packet_vector %= lit(".on_packet") > identifier > eol;

		on_periodic_vector.name("periodic vector");
		on_periodic_vector %= lit(".on_periodic") > identifier > eol;

		identifier.name("identifier");
		identifier %= lexeme[char_("_a-zA-Z") > *char_("_0-9a-zA-Z")];

		label.name("label");
		label %= identifier >> lit(":");

		instruction %=
			simple_instruction
			| ld_instruction
			| st_instruction
			| dt_masked_instruction
			| jump_instruction
			| dup_instruction
			| rot_instruction
			| switch_instruction
			| block_instruction;

		ld_instruction %=
			lit("ld")
			> (
				ld_operand_simple
				| ld_operand_immediate
				| ld_st_operand_register(hbt::ir::Opcode::LD_REG)
				| (eps > errors.ld_operand)
			);

		st_instruction %=
			lit("st")
			> ld_st_operand_register(hbt::ir::Opcode::ST_REG);

		dt_masked_instruction =
			(lit("dt.decomp") > dt_operand_mask(hbt::ir::Opcode::DT_DECOMPOSE))
			| (lit("cmp.dt.lt") > dt_operand_mask(hbt::ir::Opcode::CMP_DT_LT))
			| (lit("cmp.dt.ge") > dt_operand_mask(hbt::ir::Opcode::CMP_DT_GE));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunsequenced"
		ld_operand_immediate.name("immediate operand");
		ld_operand_immediate =
			(
				lit("u8") > lit("(")
				> u8_immed[_val = bind(make_insn_t<hbt::ir::Opcode::LD_U8>, _1)]
				> lit(")")
			) | (
				lit("u32") > lit("(")
				> u32_immed[_val = bind(make_insn_t<hbt::ir::Opcode::LD_U32>, _1)]
				> lit(")")
			) | (
				lit("f") > lit("(")
				> float_immed[_val = bind(make_insn_t<hbt::ir::Opcode::LD_FLOAT>, _1)]
				> lit(")")
			) | (
				lit("dt") > lit("(")
				> dt_immed[_val = bind(make_insn_t<hbt::ir::Opcode::LD_DT>, _1)]
				> lit(")")
			);

		jump_instruction =
			(lit("jnz") > identifier)[_val = bind(make_insn_t<hbt::ir::Opcode::JNZ>, _1)]
			| (lit("jz") > identifier)[_val = bind(make_insn_t<hbt::ir::Opcode::JZ>, _1)]
			| (lit("jump") > identifier)[_val = bind(make_insn_t<hbt::ir::Opcode::JUMP>, _1)];

		dup_instruction =
			lit("dup")
			> (
				uint_[_val = bind(make_insn_t<hbt::ir::Opcode::DUP_I>, _1)]
				| eps[_val = val(ir_instruction{ hbt::ir::Opcode::DUP, boost::none_t() })]
			);

		rot_instruction =
			lit("rot")
			> (
				uint_[_val = bind(make_insn_t<hbt::ir::Opcode::ROT_I>, _1)]
				| eps[_val = val(ir_instruction{ hbt::ir::Opcode::ROT, boost::none_t() })]
			);
#pragma GCC diagnostic pop

		switch_instruction =
			lit("switch")
			> lit("{")
			> eol
			> repeat(0, 255)[switch_entry]
				[_val = bind(make_switch, _1)]
			> (
				lit("}")
				| (eps > errors.end_of_switch)
			);

		block_instruction =
			lit("cmp.block")
			> (
				block_immed
					[_pass = bind(check_block, _1)]
					[_val = bind(make_insn_t<hbt::ir::Opcode::CMP_BLOCK>, _1)]
				| (eps > errors.binary_block_too_long)
			);

		ld_st_operand_register.name("register operand");
		ld_st_operand_register =
			(lit("[") > register_index > lit("]"))[_val = bind(make_insn, _r1, _1)];

		register_index.name("register index (0..15)");
		register_index %= uint_[_pass = _1 < 16];

		dt_operand_mask = dt_mask[_val = bind(make_insn, _r1, _1)];

		u8_immed.name("u8 immediate");
		u8_immed %= uint_[_pass = _1 <= std::numeric_limits<uint8_t>::max()];

		u32_immed.name("u32 immediate");
		u32_immed %= uint_[_pass = _1 <= std::numeric_limits<uint32_t>::max()];

		float_immed.name("float immediate");
		float_immed %= float_;

		dt_immed.name("datetime immediate");
		dt_immed %=
			(lit("s") > lit("(") > u8_immed_0_59 > lit(")"))
			^ (lit("m") > lit("(") > u8_immed_0_59 > lit(")"))
			^ (lit("h") > lit("(") > u8_immed_0_23 > lit(")"))
			^ (lit("D") > lit("(") > u8_immed_0_31 > lit(")"))
			^ (lit("M") > lit("(") > u8_immed_0_11 > lit(")"))
			^ (lit("Y") > lit("(") > u16_immed > lit(")"))
			^ (lit("W") > lit("(") > u8_immed_0_6 > lit(")"));

		u8_immed_0_59.name("0..59");
		u8_immed_0_59 %= uint_[_pass = _1 <= 59];

		u8_immed_0_31.name("0..31");
		u8_immed_0_31 %= uint_[_pass = _1 <= 31];

		u8_immed_0_23.name("0..23");
		u8_immed_0_23 %= uint_[_pass = _1 <= 23];

		u8_immed_0_11.name("0..11");
		u8_immed_0_11 %= uint_[_pass = _1 <= 11];

		u8_immed_0_6.name("0..6");
		u8_immed_0_6 %= uint_[_pass = _1 <= 6];

		u16_immed.name("0..65535");
		u16_immed %= uint_[_pass = _1 <= 65535];

		dt_mask.name("datetime mask");
		dt_mask =
			lexeme[
				char_('s')[_val |= hbt::ir::DTMask::second]
				^ char_('m')[_val |= hbt::ir::DTMask::minute]
				^ char_('h')[_val |= hbt::ir::DTMask::hour]
				^ char_('D')[_val |= hbt::ir::DTMask::day]
				^ char_('M')[_val |= hbt::ir::DTMask::month]
				^ char_('Y')[_val |= hbt::ir::DTMask::year]
				^ char_('W')[_val |= hbt::ir::DTMask::weekday]
			];

		switch_entry.name("switch table entry");
		switch_entry %= uint_ > lit(":") > identifier > eol;

		block_immed.name("immediate binary operand");
		block_immed %=
			lit("(")
			> block_start
			> lit(",")
			> block_immed_binary_operand
			> lit(")");

		block_immed_binary_operand =
			lexeme[
				lit("0x")
				> repeat(1, 32)[uint_parser<uint8_t, 16, 1, 1>()]
					[_val = bind(make_block, _1)]
			];

		block_start.name("block start position (0..15)");
		block_start %= uint_[_pass = _1 <= 15];

		// error names

		errors.binary_block_too_long.name("~binary immediate operand~block too long");
		errors.binary_block_too_long = !eps;

		errors.label_or_instruction.name("label or instruction");
		errors.label_or_instruction = !eps;

		errors.end_of_switch.name("~end of switch block~block exceeds 255 entries");
		errors.end_of_switch = !eps;

		errors.machine_id_too_long.name("~machine id~machine id too long");
		errors.machine_id_too_long = !eps;

		errors.ld_operand.name("~ld operand~immediate | src.(ip | eid | val) | sys.(state | statetime | time)");
		errors.ld_operand = !eps;
	}

	template<hbt::ir::Opcode Opcode>
	static ir_instruction make_insn_t(ir_instruction::param_t immed)
	{
		return make_insn(Opcode, immed);
	}

	static ir_instruction make_insn(hbt::ir::Opcode opcode, ir_instruction::param_t immed)
	{
		return { opcode, immed };
	}

	static ir_instruction make_switch(const std::vector<switch_entry>& entries)
	{
		return { hbt::ir::Opcode::SWITCH_32, ir_instruction::immed_t(entries) };
	}

	static bool check_block(const block_immediate& imm)
	{
		return imm.start + imm.block.size() <= 16;
	}

	static std::vector<uint8_t> make_block(const std::vector<uint8_t>& vec)
	{
		std::vector<uint8_t> result;
		int offset = 0;

		if (vec.size() % 2) {
			result.push_back(vec.front());
			offset = 1;
		}

		while (offset < vec.size()) {
			result.push_back((vec[offset] << 4) | vec[offset + 1]);
			offset += 2;
		}

		return result;
	}

	qi::rule<It, ir_program(), asm_ws<It>> start;

	qi::rule<It, ir_program_header(), asm_ws<It>> machine_header;
	qi::rule<It, std::vector<uint8_t>(), asm_ws<It>> machine_id;
	qi::rule<It, std::string(), asm_ws<It>> on_packet_vector;
	qi::rule<It, std::string(), asm_ws<It>> on_periodic_vector;

	qi::rule<It, ir_line(), asm_ws<It>> label;

	qi::rule<It, ir_line(), asm_ws<It>> instruction;

	qi::rule<It, ir_instruction(), asm_ws<It>> ld_instruction;
	qi::rule<It, ir_instruction(), asm_ws<It>> st_instruction;
	ld_simple_operands ld_operand_simple;
	qi::rule<It, ir_instruction(), asm_ws<It>> ld_operand_immediate;
	qi::rule<It, ir_instruction(), asm_ws<It>> dt_masked_instruction;
	qi::rule<It, ir_instruction(), asm_ws<It>> jump_instruction;
	qi::rule<It, ir_instruction(), asm_ws<It>> dup_instruction;
	qi::rule<It, ir_instruction(), asm_ws<It>> rot_instruction;
	qi::rule<It, ir_instruction(), asm_ws<It>> switch_instruction;
	simple_instructions simple_instruction;
	qi::rule<It, ir_instruction(), asm_ws<It>> block_instruction;

	qi::rule<It, ir_instruction(hbt::ir::Opcode), asm_ws<It>> ld_st_operand_register;
	qi::rule<It, ir_instruction(hbt::ir::Opcode), asm_ws<It>> dt_operand_mask;

	qi::rule<It, std::string(), asm_ws<It>> identifier;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed;
	qi::rule<It, uint32_t(), asm_ws<It>> u32_immed;
	qi::rule<It, float(), asm_ws<It>> float_immed;
	qi::rule<It, datetime_immediate(), asm_ws<It>> dt_immed;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_59;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_31;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_23;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_11;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_6;
	qi::rule<It, uint16_t(), asm_ws<It>> u16_immed;

	qi::rule<It, unsigned(), asm_ws<It>> register_index;

	qi::rule<It, hbt::ir::DTMask(), asm_ws<It>> dt_mask;

	qi::rule<It, switch_entry(), asm_ws<It>> switch_entry;

	qi::rule<It, unsigned(), asm_ws<It>> block_start;
	qi::rule<It, block_immediate(), asm_ws<It>> block_immed;
	qi::rule<It, std::vector<uint8_t>(), asm_ws<It>> block_immed_binary_operand;

	struct {
		qi::rule<It> binary_block_too_long;
		qi::rule<It> label_or_instruction;
		qi::rule<It> end_of_switch;
		qi::rule<It> machine_id_too_long;
		qi::rule<It> ld_operand;
	} errors;
};

template<typename Iterator>
bool parseToList(Iterator first, Iterator last)
{
	as_grammar<Iterator> g;

	bool result = qi::phrase_parse(first, last, g, asm_ws<Iterator>());

	return result && first == last;
}

}

namespace hbt {
namespace ir {

Program parse(const std::string& text)
{
	typedef boost::spirit::line_pos_iterator<const char*> lpi;

	const char* ctext = text.c_str();

	try {
		parseToList(lpi(ctext), lpi(ctext + text.size()));
	} catch (const qi::expectation_failure<lpi>& ef) {
			std::cout << std::string(ef.first, ef.last) << std::endl;
			std::string rule_name = ef.what_.tag;

			std::string expected, detail;

			if (rule_name[0] == '~') {
				size_t sep = rule_name.find('~', 1);
				expected = rule_name.substr(1, sep - 1);
				detail = rule_name.substr(sep + 1);
			} else if (rule_name == "literal-string") {
				expected = boost::get<std::string>(ef.what_.value);
			} else {
				expected = rule_name;
			}

			throw hbt::ir::ParseError(
					ef.first.position(),
					get_column(lpi(ctext), ef.first),
					expected,
					detail);
	}

	throw ParseError(0,0,"","");
}

}
}
