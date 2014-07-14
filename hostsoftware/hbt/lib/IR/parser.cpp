#include "IR/parser.hpp"

#include "IR/builder.hpp"

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

struct datetime_immediate {
	boost::optional<unsigned> second, minute, hour;
	boost::optional<unsigned> day, month, year;
	boost::optional<unsigned> weekday;
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
			datetime_immediate
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
	(boost::optional<unsigned>, second)
	(boost::optional<unsigned>, minute)
	(boost::optional<unsigned>, hour)
	(boost::optional<unsigned>, day)
	(boost::optional<unsigned>, month)
	(boost::optional<unsigned>, year)
	(boost::optional<unsigned>, weekday)
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
			(standard::space - eol)
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
	as_grammar(std::string& badToken)
		: as_grammar::base_type(start, "HBT IR program")
	{
		using namespace qi;
		using boost::phoenix::bind;
		using boost::phoenix::val;

		start %=
			machine_header
			> *(
				(label > eol)
				| (instruction > (eol | !!eoi))
				| eol
				| (!eoi > errors.label_or_instruction)
			)
			> eoi;

		machine_header.name("machine header");
		machine_header %=
			(lit(".version") > lit("0") > +eol)
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
				> (+eol | (eps > errors.machine_id_too_long))
			)[_val = bind(make_block, _1)];

		on_packet_vector.name("packet vector");
		on_packet_vector %= lit(".on_packet") > identifier > +eol;

		on_periodic_vector.name("periodic vector");
		on_periodic_vector %= lit(".on_periodic") > identifier > +eol;

		identifier.name("identifier");
		identifier %= lexeme[char_("_a-zA-Z") > *char_("_0-9a-zA-Z")];

		label.name("label");
		label %= identifier >> lit(":");

#define TOKEN(p) (lexeme[p >> token_end])

		instruction %=
			TOKEN(simple_instruction)
			| ld_instruction
			| st_instruction
			| dt_masked_instruction
			| jump_instruction
			| dup_instruction
			| rot_instruction
			| switch_instruction
			| block_instruction;

		ld_instruction %=
			TOKEN("ld")
			> (
				ld_operand_simple
				| ld_operand_immediate
				| ld_st_operand_register(hbt::ir::Opcode::LD_REG)
				| (eps > errors.ld_operand)
			);

		st_instruction %=
			TOKEN("st")
			> ld_st_operand_register(hbt::ir::Opcode::ST_REG);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunsequenced"
		dt_masked_instruction =
			(TOKEN("dt.decomp") > dt_mask[_val = bind(make_insn_t<hbt::ir::Opcode::DT_DECOMPOSE>, _1)])
			| (TOKEN("cmp.dt.lt") > dt_mask[_val = bind(make_insn_t<hbt::ir::Opcode::CMP_DT_LT>, _1)])
			| (TOKEN("cmp.dt.ge") > dt_mask[_val = bind(make_insn_t<hbt::ir::Opcode::CMP_DT_GE>, _1)]);

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
			(TOKEN("jnz") > identifier)[_val = bind(make_insn_t<hbt::ir::Opcode::JNZ>, _1)]
			| (TOKEN("jz") > identifier)[_val = bind(make_insn_t<hbt::ir::Opcode::JZ>, _1)]
			| (TOKEN("jump") > identifier)[_val = bind(make_insn_t<hbt::ir::Opcode::JUMP>, _1)];

		dup_instruction =
			TOKEN("dup")
			> (
				stack_slot[_val = bind(make_insn_t<hbt::ir::Opcode::DUP_I>, _1)]
				| eps[_val = val(ir_instruction{ hbt::ir::Opcode::DUP, boost::none_t() })]
			);

		rot_instruction =
			TOKEN("rot")
			> (
				stack_slot[_val = bind(make_insn_t<hbt::ir::Opcode::ROT_I>, _1)]
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

		stack_slot.name("stack slot (0..31)");
		stack_slot %= uint_[_pass = _1 < 32];

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

#undef TOKEN

		token_end = no_skip[!!(space | eol | eoi | standard::punct)];

		// error names

		auto storeBadToken =
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
		errors.label_or_instruction = storeBadToken >> !eps;

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

	qi::rule<It, std::string(), asm_ws<It>> identifier;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed;
	qi::rule<It, uint32_t(), asm_ws<It>> u32_immed;
	qi::rule<It, float(), asm_ws<It>> float_immed;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_59;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_31;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_23;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_11;
	qi::rule<It, uint8_t(), asm_ws<It>> u8_immed_0_6;
	qi::rule<It, uint16_t(), asm_ws<It>> u16_immed;

	qi::rule<It, uint8_t(), asm_ws<It>> register_index;
	qi::rule<It, uint8_t(), asm_ws<It>> stack_slot;

	qi::rule<It, hbt::ir::DTMask(), asm_ws<It>> dt_mask;
	qi::rule<It, datetime_immediate(), asm_ws<It>> dt_immed;

	qi::rule<It, switch_entry(), asm_ws<It>> switch_entry;

	qi::rule<It, unsigned(), asm_ws<It>> block_start;
	qi::rule<It, block_immediate(), asm_ws<It>> block_immed;
	qi::rule<It, std::vector<uint8_t>(), asm_ws<It>> block_immed_binary_operand;

	asm_ws<It> space;
	qi::rule<It> token_end;

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

void ensureUniqueLabelsIn(const ir_program& program)
{
	std::set<std::string> defined;
	std::string duplicates;

	for (const auto& line : program.lines) {
		if (line.which() != 0)
			continue;

		const auto& label = boost::get<std::string>(line);

		if (!defined.insert(label).second) {
			if (duplicates.size())
				duplicates += ", ";
			duplicates += label;
		}
	}

	if (duplicates.size())
		throw hbt::ir::InvalidProgram("duplicate labels: " + duplicates);
}

std::array<uint8_t, 16> toMachineID(const std::vector<uint8_t>& vec)
{
	std::array<uint8_t, 16> result;

	result.fill(0);
	std::copy(vec.begin(), vec.end(), result.begin() + 16 - vec.size());
	return result;
}

std::map<std::string, hbt::ir::Label> makeLabelMap(const ir_program& program, hbt::ir::Builder& builder)
{
	std::map<std::string, hbt::ir::Label> result;

	auto it = program.lines.begin();
	auto end = program.lines.end();

	while (it != end) {
		if (it->which() == 0) {
			hbt::ir::Label current = builder.createLabel();

			while (it != end && it->which() == 0) {
				result.insert({ boost::get<std::string>(*it), current });
				++it;
			}
		} else {
			++it;
		}
	}

	return result;
}

std::unique_ptr<hbt::ir::Program> makeProgram(const ir_program& program)
{
	using namespace hbt::ir;

	Builder builder(0, toMachineID(program.header.machine_id));

	std::map<std::string, Label> labels = makeLabelMap(program, builder);
	std::set<std::string> marked;

	auto useLabel = [&labels, &marked, &builder] (const std::string& l) -> Label& {
		if (marked.count(l))
			throw InvalidProgram("backward jump to " + l + " not allowed");
		return labels.at(l);
	};

	Label* nextLabel = nullptr;
	for (const auto& line : program.lines) {
		if (line.which() == 0) {
			const std::string& l = boost::get<std::string>(line);
			labels.insert({ l, builder.createLabel() });
			marked.insert(l);
			nextLabel = &labels.at(l);
			continue;
		}

		boost::optional<Label> thisLabel;
		if (nextLabel)
			thisLabel = *nextLabel;
		nextLabel = nullptr;

		const ir_instruction& insn = boost::get<ir_instruction>(line);
		if (!insn.immediate) {
			builder.append(thisLabel, insn.opcode);
			continue;
		}

		switch (insn.immediate->which()) {
		case 0:
			builder.append(thisLabel, insn.opcode, boost::get<uint8_t>(*insn.immediate));
			break;

		case 1:
			builder.append(thisLabel, insn.opcode, boost::get<uint32_t>(*insn.immediate));
			break;

		case 2:
			builder.append(thisLabel, insn.opcode, boost::get<float>(*insn.immediate));
			break;

		case 3: {
			const auto& operand = boost::get<std::vector<switch_entry>>(*insn.immediate);
			std::vector<SwitchEntry> entries;

			entries.reserve(operand.size());
			std::transform(operand.begin(), operand.end(), std::back_inserter(entries),
				[&useLabel](const switch_entry& e) {
					return SwitchEntry{ e.label, useLabel(e.target) };
				});

			SwitchTable table(entries.begin(), entries.end());

			builder.append(thisLabel, insn.opcode, std::move(table));

			break;
		}

		case 4: {
			const auto& operand = boost::get<block_immediate>(*insn.immediate);

			std::array<uint8_t, 16> data;

			data.fill(0);
			std::copy(operand.block.begin(), operand.block.end(), data.begin());

			BlockPart block(operand.start, operand.block.size(), data);

			builder.append(thisLabel, insn.opcode, block);

			break;
		}

		case 5:
			builder.append(thisLabel, insn.opcode, boost::get<DTMask>(*insn.immediate));
			break;

		case 6:
			builder.append(thisLabel, insn.opcode, useLabel(boost::get<std::string>(*insn.immediate)));
			break;

		case 7: {
			const auto& operand = boost::get<datetime_immediate>(*insn.immediate);

			unsigned s, m, h, D, M, Y, W;
			DTMask mask = DTMask(0);

			auto checkFlag = [&] (DTMask flag, unsigned& t, boost::optional<unsigned> val) {
				t = val.get_value_or(0);
				if (val)
					mask |= flag;
			};

			checkFlag(DTMask::second, s, operand.second);
			checkFlag(DTMask::minute, m, operand.minute);
			checkFlag(DTMask::hour, h, operand.hour);
			checkFlag(DTMask::day, D, operand.day);
			checkFlag(DTMask::month, M, operand.month);
			checkFlag(DTMask::year, Y, operand.year);
			checkFlag(DTMask::weekday, W, operand.weekday);

			DateTime dt(s, m, h, D, M, Y, W);

			builder.append(thisLabel, insn.opcode, std::make_tuple(mask, dt));
			break;
		}

		default:
			throw std::runtime_error("internal error: invalid assembler program?");
		}
	}

	if (labels.count(program.header.on_packet))
		builder.onPacket(labels.at(program.header.on_packet));
	if (labels.count(program.header.on_periodic))
		builder.onPeriodic(labels.at(program.header.on_periodic));

	return builder.finish();
}

}

namespace hbt {
namespace ir {

std::unique_ptr<Program> parse(const std::string& text)
{
	typedef boost::spirit::line_pos_iterator<const char*> lpi;

	const char* ctext = text.c_str();
	std::string badToken;

	ir_program parsed;

	try {
		if (!parseToList(lpi(ctext), lpi(ctext + text.size()), parsed, badToken))
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

			throw hbt::ir::ParseError(
					ef.first.position(),
					get_column(lpi(ctext), ef.first),
					expected,
					detail);
	}

	ensureUniqueLabelsIn(parsed);
	return makeProgram(parsed);
}

}
}
