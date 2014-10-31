#include "MC/parser.hpp"

#include "MC/builder.hpp"
#include "MC/program.hpp"
#include "Util/memorybuffer.hpp"

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/tuple/tuple.hpp>

#include <algorithm>
#include <iterator>
#include <map>
#include <tuple>

namespace {

struct switch_entry {
	uint32_t label;
	std::string target;
};

struct block_immediate {
	unsigned start;
	std::vector<uint8_t> block;
};

struct mem_immediate {
	hbt::mc::MemType type;
	uint16_t addr;
};

struct ir_instruction {
	typedef boost::variant<
			uint8_t,
			uint16_t,
			uint32_t,
			uint64_t,
			int8_t,
			int16_t,
			int32_t,
			int64_t,
			float,
			std::vector<switch_entry>,
			block_immediate,
			hbt::mc::DTMask,
			std::string,
			mem_immediate
		> param_t;

	typedef boost::optional<param_t> immed_t;

	hbt::mc::Opcode opcode;
	immed_t immediate;
};

struct ir_line {
	unsigned line;
	boost::variant<std::string, ir_instruction> content;
};

struct ir_program_header {
	std::string on_packet, on_periodic, on_init;
};

struct ir_program {
	ir_program_header header;

	std::vector<ir_line> lines;
};

}

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
	(hbt::mc::Opcode, opcode)
	(ir_instruction::immed_t, immediate)
)

BOOST_FUSION_ADAPT_STRUCT(
	ir_line,
	(unsigned, line)
	(decltype(std::declval<ir_line>().content), content)
)

BOOST_FUSION_ADAPT_STRUCT(
	ir_program_header,
	(std::string, on_init)
	(std::string, on_packet)
	(std::string, on_periodic)
)

BOOST_FUSION_ADAPT_STRUCT(
	ir_program,
	(ir_program_header, header)
	(std::vector<ir_line>, lines)
)

namespace hbt {
namespace mc {

namespace {

namespace qi = boost::spirit::qi;

template<typename It>
struct asm_ws : qi::grammar<It> {
	asm_ws() : asm_ws::base_type(start)
	{
		using namespace qi;

		start =
			(standard::space - eol)
			| (lit(";") >> *(char_ - eol));
	}

	qi::rule<It> start;
};

struct simple_instructions : qi::symbols<char, ir_instruction> {
	simple_instructions()
	{
		add
			("mul", { Opcode::MUL, boost::none_t() })
			("div", { Opcode::DIV, boost::none_t() })
			("mod", { Opcode::MOD, boost::none_t() })
			("add", { Opcode::ADD, boost::none_t() })
			("sub", { Opcode::SUB, boost::none_t() })
			("and", { Opcode::AND, boost::none_t() })
			("or", { Opcode::OR, boost::none_t() })
			("xor", { Opcode::XOR, boost::none_t() })
			("shl", { Opcode::SHL, boost::none_t() })
			("shr", { Opcode::SHR, boost::none_t() })
			("cmp.lt", { Opcode::CMP_LT, boost::none_t() })
			("cmp.le", { Opcode::CMP_LE, boost::none_t() })
			("cmp.gt", { Opcode::CMP_GT, boost::none_t() })
			("cmp.ge", { Opcode::CMP_GE, boost::none_t() })
			("cmp.eq", { Opcode::CMP_EQ, boost::none_t() })
			("cmp.neq", { Opcode::CMP_NEQ, boost::none_t() })
			("conv.b", { Opcode::CONV_B, boost::none_t() })
			("conv.u8", { Opcode::CONV_U8, boost::none_t() })
			("conv.u16", { Opcode::CONV_U16, boost::none_t() })
			("conv.u32", { Opcode::CONV_U32, boost::none_t() })
			("conv.u64", { Opcode::CONV_U64, boost::none_t() })
			("conv.s8", { Opcode::CONV_S8, boost::none_t() })
			("conv.s16", { Opcode::CONV_S16, boost::none_t() })
			("conv.s32", { Opcode::CONV_S32, boost::none_t() })
			("conv.s64", { Opcode::CONV_S64, boost::none_t() })
			("conv.f", { Opcode::CONV_F, boost::none_t() })
			("write", { Opcode::WRITE, boost::none_t() })
			("ret", { Opcode::RET, boost::none_t() });
	}
};

struct ld_simple_operands : qi::symbols<char, ir_instruction> {
	ld_simple_operands()
	{
		add
			("src.eid", { Opcode::LD_SOURCE_EID, boost::none_t() })
			("src.val", { Opcode::LD_SOURCE_VAL, boost::none_t() })
			("false", { Opcode::LD_FALSE, boost::none_t() })
			("true", { Opcode::LD_TRUE, boost::none_t() })
			("sys.time", { Opcode::LD_SYSTIME, boost::none_t() });
	}
};

template<typename It>
struct as_grammar : qi::grammar<It, ir_program(), asm_ws<It>> {
	as_grammar(std::string& badToken)
		: as_grammar::base_type(start, "HBT IR program")
	{
		using namespace qi;
		using boost::phoenix::bind;
		using boost::phoenix::val;

		start %=
			machine_header
			> *(
				(label > +eol)
				| (instruction > (+eol | !!eoi))
				| (!eoi > errors.label_or_instruction)
			)
			> eoi;

		machine_header.name("machine header");
		machine_header %=
			*eol
			> (lit(".version") > lit("0") > +eol)
			> on_init_vector
			> on_packet_vector
			> on_periodic_vector;

		on_packet_vector.name("packet vector");
		on_packet_vector %=
			(lit(".on_packet") > identifier > +eol)
			| string("");

		on_periodic_vector.name("periodic vector");
		on_periodic_vector %=
			(lit(".on_periodic") > identifier > +eol)
			| string("");

		on_init_vector.name("init vector");
		on_init_vector %=
			(lit(".on_init") > identifier > +eol)
			| string("");

		identifier.name("identifier");
		identifier %= lexeme[char_("_a-zA-Z") > *char_("_0-9a-zA-Z.")];

		label.name("label");
		label %= currentLine >> identifier >> lit(":");

#define TOKEN(p) (lexeme[p >> token_end])

		instruction %=
			currentLine
			>> (
				TOKEN(simple_instruction)
				| ld_instruction
				| st_instruction
				| dt_masked_instruction
				| jump_instruction
				| dup_instruction
				| rot_instruction
				| pop_instruction
				| switch_instruction
				| block_instruction
			);

		ld_instruction %=
			TOKEN("ld")
			> (
				ld_operand_simple
				| ld_operand_immediate
				| mem_instruction_rest(Opcode::LD_MEM)
				| (eps > errors.ld_operand)
			);

		st_instruction %=
			TOKEN("st")
			> mem_instruction_rest(Opcode::ST_MEM);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsequenced"
		dt_masked_instruction =
			(TOKEN("dt.decomp") > dt_mask[_val = bind(make_insn_t<Opcode::DT_DECOMPOSE>, _1)]);

		ld_operand_immediate.name("immediate operand");
		ld_operand_immediate =
			(
				(lit("u8") >> lit("("))
				> u8_immed[_val = bind(make_insn_t<Opcode::LD_U8>, _1)]
				> lit(")")
			) | (
				(lit("u16") >> lit("("))
				> u16_immed[_val = bind(make_insn_t<Opcode::LD_U16>, _1)]
				> lit(")")
			) | (
				(lit("u32") >> lit("("))
				> u32_immed[_val = bind(make_insn_t<Opcode::LD_U32>, _1)]
				> lit(")")
			) | (
				(lit("u64") >> lit("("))
				> u64_immed[_val = bind(make_insn_t<Opcode::LD_U64>, _1)]
				> lit(")")
			) | (
				(lit("s8") >> lit("("))
				> s8_immed[_val = bind(make_insn_t<Opcode::LD_S8>, _1)]
				> lit(")")
			) | (
				(lit("s16") >> lit("("))
				> s16_immed[_val = bind(make_insn_t<Opcode::LD_S16>, _1)]
				> lit(")")
			) | (
				(lit("s32") >> lit("("))
				> s32_immed[_val = bind(make_insn_t<Opcode::LD_S32>, _1)]
				> lit(")")
			) | (
				(lit("s64") >> lit("("))
				> s64_immed[_val = bind(make_insn_t<Opcode::LD_S64>, _1)]
				> lit(")")
			) | (
				(lit("f") >> lit("("))
				> float_immed[_val = bind(make_insn_t<Opcode::LD_FLOAT>, _1)]
				> lit(")")
			);

		jump_instruction =
			(TOKEN("jnz") > identifier)[_val = bind(make_insn_t<Opcode::JNZ>, _1)]
			| (TOKEN("jz") > identifier)[_val = bind(make_insn_t<Opcode::JZ>, _1)]
			| (TOKEN("jump") > identifier)[_val = bind(make_insn_t<Opcode::JUMP>, _1)];

		dup_instruction =
			TOKEN("dup")
			> (
				stack_slot[_val = bind(make_insn_t<Opcode::DUP_I>, _1)]
				| eps[_val = val(ir_instruction{ Opcode::DUP, boost::none_t() })]
			);

		rot_instruction =
			TOKEN("rot")
			> (
				stack_slot[_val = bind(make_insn_t<Opcode::ROT_I>, _1)]
				| eps[_val = val(ir_instruction{ Opcode::ROT, boost::none_t() })]
			);

		pop_instruction =
			TOKEN("pop")
			> (
				stack_slot[_val = bind(make_insn_t<Opcode::POP_I>, _1)]
				| eps[_val = val(ir_instruction{ Opcode::POP, boost::none_t() })]
			);

		switch_instruction =
			(
				lit("switch8")[_a = Opcode::SWITCH_8]
				| lit("switch16")[_a = Opcode::SWITCH_16]
				| lit("switch32")[_a = Opcode::SWITCH_32]
			) > lit("{")
			> +eol
			> repeat(0, 255)[switch_table_entry]
				[_val = bind(make_switch, _1, _a)]
			> (
				lit("}")
				| (eps > errors.end_of_switch)
			);

		block_instruction =
			lit("cmp.srcip")
			> (
				block_immed
					[_pass = bind(check_block, _1)]
					[_val = bind(make_insn_t<Opcode::CMP_SRC_IP>, _1)]
				| (eps > errors.binary_block_too_long)
			);

		mem_instruction_rest.name("memory operand");
		mem_instruction_rest =
			(
				((lit("b") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::Bool, _1)]
				| ((lit("u8") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::U8, _1)]
				| ((lit("u16") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::U16, _1)]
				| ((lit("u32") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::U32, _1)]
				| ((lit("u64") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::U64, _1)]
				| ((lit("s8") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::S8, _1)]
				| ((lit("s16") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::S16, _1)]
				| ((lit("s32") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::S32, _1)]
				| ((lit("s64") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::S64, _1)]
				| ((lit("f") >> lit("[")) > mem_addr)[_val = bind(make_mem_insn, _r1, MemType::Float, _1)]
			) > lit("]");
#pragma clang diagnostic pop

		mem_addr.name("memory address (0..4095)");
		mem_addr %= uint_[_pass = _1 < 4096];

		stack_slot.name("stack slot (0..255)");
		stack_slot %= uint_[_pass = _1 < 256];

		u8_immed.name("u8 immediate");
		u8_immed %= ulong_long[_pass = _1 <= std::numeric_limits<uint8_t>::max()];

		u16_immed.name("u16 immediate");
		u16_immed %= ulong_long[_pass = _1 <= std::numeric_limits<uint16_t>::max()];

		u32_immed.name("u32 immediate");
		u32_immed %= ulong_long[_pass = _1 <= std::numeric_limits<uint32_t>::max()];

		u64_immed.name("u64 immediate");
		u64_immed %= ulong_long[_pass = _1 <= std::numeric_limits<uint64_t>::max()];

		s8_immed.name("s8 immediate");
		s8_immed %= long_long[_pass = _1 >= std::numeric_limits<int8_t>::min() && _1 <= std::numeric_limits<int8_t>::max()];

		s16_immed.name("s16 immediate");
		s16_immed %= long_long[_pass = _1 >= std::numeric_limits<int16_t>::min() && _1 <= std::numeric_limits<int16_t>::max()];

		s32_immed.name("s32 immediate");
		s32_immed %= long_long[_pass = _1 >= std::numeric_limits<int32_t>::min() && _1 <= std::numeric_limits<int32_t>::max()];

		s64_immed.name("s64 immediate");
		s64_immed %= long_long[_pass = _1 >= std::numeric_limits<int64_t>::min() && _1 <= std::numeric_limits<int64_t>::max()];

		float_immed.name("float immediate");
		float_immed %= float_;

		dt_mask.name("datetime mask");
		dt_mask =
			lexeme[
				char_('s')[_val |= DTMask::second]
				^ char_('m')[_val |= DTMask::minute]
				^ char_('h')[_val |= DTMask::hour]
				^ char_('D')[_val |= DTMask::day]
				^ char_('M')[_val |= DTMask::month]
				^ char_('Y')[_val |= DTMask::year]
				^ char_('W')[_val |= DTMask::weekday]
			];

		switch_table_entry.name("switch table entry");
		switch_table_entry %= uint_ > lit(":") > identifier > +eol;

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

		currentLine = raw[eps][_val = bind(getLineNumber, _1)];

#undef TOKEN

		token_end = no_skip[!!(space | eol | eoi | standard::punct)];

		// error names

		store_bad_token =
			-(
				lexeme[+(standard::print - ';' - standard::space)]
					[([&badToken] (const std::vector<char>& s, qi::unused_type, qi::unused_type) {
						badToken = "got ";
						badToken.append(s.begin(), s.end());
					})]
			);

		errors.binary_block_too_long.name("~binary immediate operand~block too long");
		errors.binary_block_too_long = !eps;

		errors.label_or_instruction.name("label or instruction");
		errors.label_or_instruction = store_bad_token >> !eps;

		errors.end_of_switch.name("~end of switch block~block exceeds 255 entries");
		errors.end_of_switch = !eps;

		errors.machine_id_too_long.name("~machine id~machine id too long");
		errors.machine_id_too_long = !eps;

		errors.ld_operand.name("~ld operand~immediate | src.(eid | val) | sys.time");
		errors.ld_operand = !eps;
	}

	static unsigned getLineNumber(const boost::iterator_range<It>& range)
	{
		return get_line(range.begin());
	}

	template<Opcode Opcode>
	static ir_instruction make_insn_t(ir_instruction::param_t immed)
	{
		return make_insn(Opcode, immed);
	}

	static ir_instruction make_insn(Opcode opcode, ir_instruction::param_t immed)
	{
		return { opcode, immed };
	}

	static ir_instruction make_mem_insn(Opcode opcode, MemType type, uint16_t addr)
	{
		return make_insn(opcode, mem_immediate{type, addr});
	}

	static ir_instruction make_switch(const std::vector<switch_entry>& entries, Opcode op)
	{
		return { op, ir_instruction::immed_t(entries) };
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
	qi::rule<It, std::string(), asm_ws<It>> on_packet_vector;
	qi::rule<It, std::string(), asm_ws<It>> on_periodic_vector;
	qi::rule<It, std::string(), asm_ws<It>> on_init_vector;

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
	qi::rule<It, ir_instruction(), asm_ws<It>> pop_instruction;
	qi::rule<It, ir_instruction(), qi::locals<Opcode>, asm_ws<It>> switch_instruction;
	simple_instructions simple_instruction;
	qi::rule<It, ir_instruction(), asm_ws<It>> block_instruction;

	qi::rule<It, ir_instruction(Opcode), asm_ws<It>> mem_instruction_rest;

	qi::rule<It, std::string(), asm_ws<It>> identifier;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed;
	qi::rule<It, uint16_t(), asm_ws<It>> u16_immed;
	qi::rule<It, uint32_t(), asm_ws<It>> u32_immed;
	qi::rule<It, uint64_t(), asm_ws<It>> u64_immed;
	qi::rule<It, int8_t(), asm_ws<It>> s8_immed;
	qi::rule<It, int16_t(), asm_ws<It>> s16_immed;
	qi::rule<It, int32_t(), asm_ws<It>> s32_immed;
	qi::rule<It, int64_t(), asm_ws<It>> s64_immed;
	qi::rule<It, float(), asm_ws<It>> float_immed;

	qi::rule<It, uint16_t(), asm_ws<It>> mem_addr;
	qi::rule<It, uint8_t(), asm_ws<It>> stack_slot;

	qi::rule<It, DTMask(), asm_ws<It>> dt_mask;

	qi::rule<It, switch_entry(), asm_ws<It>> switch_table_entry;

	qi::rule<It, unsigned(), asm_ws<It>> block_start;
	qi::rule<It, block_immediate(), asm_ws<It>> block_immed;
	qi::rule<It, std::vector<uint8_t>(), asm_ws<It>> block_immed_binary_operand;

	qi::rule<It, unsigned()> currentLine;

	asm_ws<It> space;
	qi::rule<It> token_end;
	qi::rule<It> store_bad_token;

	struct {
		qi::rule<It, asm_ws<It>> binary_block_too_long;
		qi::rule<It, asm_ws<It>> label_or_instruction;
		qi::rule<It, asm_ws<It>> end_of_switch;
		qi::rule<It, asm_ws<It>> machine_id_too_long;
		qi::rule<It, asm_ws<It>> ld_operand;
	} errors;
};

template<typename Iterator>
bool parseToList(Iterator first, Iterator last, ir_program& program, std::string& badToken)
{
	as_grammar<Iterator> g(badToken);

	bool result = qi::phrase_parse(first, last, g, asm_ws<Iterator>(), program);

	return result && first == last;
}

std::array<uint8_t, 16> toMachineID(const std::vector<uint8_t>& vec)
{
	std::array<uint8_t, 16> result;

	result.fill(0);
	std::copy(vec.begin(), vec.end(), result.begin() + 16 - vec.size());
	return result;
}

std::map<std::string, Label> makeLabelMap(const ir_program& program, Builder& builder)
{
	std::map<std::string, Label> result;

	auto it = program.lines.begin();
	auto end = program.lines.end();

	while (it != end) {
		if (it->content.which() == 0) {
			Label current = builder.createLabel(boost::get<std::string>(it->content));

			while (it != end && it->content.which() == 0) {
				result.insert({ boost::get<std::string>(it->content), current });
				++it;
			}
		} else {
			++it;
		}
	}

	return result;
}

std::unique_ptr<Program> makeProgram(const ir_program& program)
{
	using namespace hbt::mc;

	Builder builder(0);

	std::map<std::string, Label> labels = makeLabelMap(program, builder);

	auto getLabelFor = [&labels, &builder] (const std::string& id) -> Label& {
		if (!labels.count(id))
			labels.insert({ id, builder.createLabel(id) });
		return labels.at(id);
	};

	Label* nextLabel = nullptr;
	for (const auto& line : program.lines) {
		if (line.content.which() == 0) {
			auto&& l = boost::get<std::string>(line.content);
			nextLabel = &getLabelFor(l);
			continue;
		}

		boost::optional<Label> thisLabel;
		if (nextLabel)
			thisLabel = *nextLabel;
		nextLabel = nullptr;

		const ir_instruction& insn = boost::get<ir_instruction>(line.content);
		if (!insn.immediate) {
			builder.insert(thisLabel, insn.opcode, line.line);
			continue;
		}

		using boost::get;
		if (auto* val = get<uint8_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<uint16_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<uint32_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<uint64_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<int8_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<int16_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<int32_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<int64_t>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<float>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<std::vector<switch_entry>>(insn.immediate.get_ptr())) {
			std::vector<SwitchEntry> entries;

			entries.reserve(val->size());
			std::transform(val->begin(), val->end(), std::back_inserter(entries),
				[&getLabelFor](const switch_entry& e) {
					return SwitchEntry{ e.label, getLabelFor(e.target) };
				});

			SwitchTable table(entries.begin(), entries.end());

			builder.insert(thisLabel, insn.opcode, std::move(table), line.line);
		} else if (auto* val = get<block_immediate>(insn.immediate.get_ptr())) {
			std::array<uint8_t, 16> data;

			data.fill(0);
			std::copy(val->block.begin(), val->block.end(), data.begin());

			BlockPart block(val->start, val->block.size(), data);

			builder.insert(thisLabel, insn.opcode, block, line.line);
		} else if (auto* val = get<DTMask>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, *val, line.line);
		} else if (auto* val = get<std::string>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, getLabelFor(*val), line.line);
		} else if (auto* val = get<mem_immediate>(insn.immediate.get_ptr())) {
			builder.insert(thisLabel, insn.opcode, std::make_tuple(val->type, val->addr), line.line);
		} else {
			throw std::runtime_error("internal error: invalid assembler program?");
		}
	}

	if (program.header.on_packet.size())
		builder.onPacket(getLabelFor(program.header.on_packet));
	if (program.header.on_periodic.size())
		builder.onPeriodic(getLabelFor(program.header.on_periodic));
	if (program.header.on_init.size())
		builder.onInit(getLabelFor(program.header.on_init));

	return builder.finish();
}

}

std::unique_ptr<Program> parse(const hbt::util::MemoryBuffer& input)
{
	typedef boost::spirit::line_pos_iterator<const char*> lpi;

	auto ctext = input.range<char>();
	std::string badToken;

	ir_program parsed;

	try {
		if (!parseToList(lpi(ctext.begin()), lpi(ctext.end()), parsed, badToken))
			throw ParseError(0, 0, "...not this anyway...", "parsing aborted (internal error)");
	} catch (const qi::expectation_failure<lpi>& ef) {
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
				if (badToken.size())
					detail = badToken;
			}

			throw ParseError(
					ef.first.position(),
					get_column(lpi(ctext.begin()), ef.first),
					expected,
					detail);
	}

	return makeProgram(parsed);
}

}
}
