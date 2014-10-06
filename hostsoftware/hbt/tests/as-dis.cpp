#define BOOST_TEST_MODULE assembler/disassembler tests
#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <vector>

#include "IR/parser.hpp"
#include "IR/program.hpp"
#include "IR/program_printer.hpp"
#include "MC/assembler.hpp"
#include "MC/disassembler.hpp"
#include "Util/memorybuffer.hpp"

namespace {

struct AssemblerLine {
	const char* textual;
	std::vector<uint8_t> binary;
	bool ignoreTextOnDisassembly;
};

struct Testcase {
	std::vector<AssemblerLine> lines;

	Testcase(std::initializer_list<AssemblerLine> lines)
		: lines(lines)
	{}

	Testcase(std::initializer_list<const char*> lines)
	{
		this->lines.reserve(lines.size());
		for (auto line : lines)
			this->lines.push_back({ line });
	}

	operator const std::vector<AssemblerLine>&() const
	{
		return lines;
	}
};

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

static Testcase allFeatures = {
	{ ".version 0" },
	{ ".machine 0x1234567890abcdeffedcba0987654321" },
	{ "; machine id",
		{ 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x09, 0x87, 0x65, 0x43, 0x21 },
		true },
	{ "; version number", { 0x0 }, true },
	{ ".on_init L0", { 0x00, 0x07 } },
	{ ".on_packet L1", { 0x00, 0x08 } },
	{ ".on_periodic L0", { 0x00, 0x07 } },
	{ "" },
	{ "L0:" },
	{ "	ret", { 0x33 } },
	{ "L1:" },
	{ "	ld src.ip", { 0x00 } },
	{ "	ld src.eid", { 0x01 } },
	{ "	ld src.val", { 0x02 } },
	{ "	ld false", { 0x03 } },
	{ "	ld true", { 0x04 } },
	{ "	ld u8(42)", { 0x05, 42 } },
	{ "	ld u32(65535)", { 0x06, 0xff, 0xff } },
	{ "	ld u32(65536)", { 0x07, 0, 1, 0, 0 } },
	{ "	ld u64(18446744073709551615)", { 0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } },
	{ "	ld f(4.2)", { 0x09, 0x40, 0x86, 0x66, 0x66 } },
	{ "	ld sys.time", { 0x0a } },
	{ "	ld b[258]", { 0x0b, 0x01, 0x02 } },
	{ "	st b[258]", { 0x0c, 0x01, 0x02 } },
	{ "	ld u8[258]", { 0x0b, 0x11, 0x02 } },
	{ "	st u8[258]", { 0x0c, 0x11, 0x02 } },
	{ "	ld u32[258]", { 0x0b, 0x21, 0x02 } },
	{ "	st u32[258]", { 0x0c, 0x21, 0x02 } },
	{ "	ld u64[258]", { 0x0b, 0x31, 0x02 } },
	{ "	st u64[258]", { 0x0c, 0x31, 0x02 } },
	{ "	ld f[258]", { 0x0b, 0x41, 0x02 } },
	{ "	st f[258]", { 0x0c, 0x41, 0x02 } },
	{ "	mul", { 0x0d } },
	{ "	div", { 0x0e } },
	{ "	mod", { 0x0f } },
	{ "	add", { 0x10 } },
	{ "	sub", { 0x11 } },
	{ "	and", { 0x12 } },
	{ "	or", { 0x13 } },
	{ "	xor", { 0x14 } },
	{ "	not", { 0x15 } },
	{ "	shl", { 0x16 } },
	{ "	shr", { 0x17 } },
	{ "	dup", { 0x18 } },
	{ "	dup 3", { 0x19, 3 } },
	{ "	rot", { 0x1a } },
	{ "	rot 3", { 0x1b, 3 } },
	{ "	dt.decomp shMW", { 0x1c, 0x55 } },
	{ "	switch {", { 0x1d, 2 } },
	{ "		0: L2", { 0, /*to*/ 0, 24 } },
	{ "		1: L3", { 1, /*to*/ 0, 28 } },
	{ "	}" },
	{ "	switch {", { 0x1e, 2 } },
	{ "		65534: L2", { 0xff, 0xfe, /*to*/ 0, 14 } },
	{ "		65535: L3", { 0xff, 0xff, /*to*/ 0, 18 } },
	{ "	}" },
	{ "	switch {", { 0x1f, 2 } },
	{ "		65536: L2", { 0, 1, 0, 0, /*to*/ 0, 0 } },
	{ "		65537: L3", { 0, 1, 0, 1, /*to*/ 0, 4 } },
	{ "	}" },
	{ "L2:" },
	{ "	cmp.block (0, 0x0809)", { 0x20, 0x01, 0x08, 0x09 } },
	{ "L3:" },
	{ "	cmp.localhost", { 0x21 } },
	{ "	cmp.lt", { 0x22 } },
	{ "	cmp.le", { 0x23 } },
	{ "	cmp.gt", { 0x24 } },
	{ "	cmp.ge", { 0x25 } },
	{ "	cmp.eq", { 0x26 } },
	{ "	cmp.neq", { 0x27 } },
	{ "	conv.b", { 0x28 } },
	{ "	conv.u8", { 0x29 } },
	{ "	conv.u32", { 0x2a } },
	{ "	conv.u64", { 0x2b } },
	{ "	conv.f", { 0x2c } },
	{ "	jnz L4", { 0x2d, 0, 6 } },
	{ "	jz L4", { 0x2e, 0, 3 } },
	{ "	jump L4", { 0x2f, 0, 0 } },
	{ "L4:" },
	{ "	write", { 0x30 } },
	{ "	pop", { 0x31 } },
	{ "	exchange 7", { 0x32, 0x07 } },
	{ "	ret", { 0x33 } },
};

static Testcase noInit = {
	{ ".version 0" },
	{ ".machine 0x00000000000000000000000000000000" },
	{ "; machine id", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, true },
	{ "; version number", { 0x0 }, true },
	{ "; no on_init", { 0xff, 0xff }, true },
	{ ".on_packet L0", { 0x00, 0x07 } },
	{ ".on_periodic L0", { 0x00, 0x07 } },
	{ "" },
	{ "L0:" },
	{ "	ret", { 0x33 } },
};

static Testcase multiLabel = {
	".version 0",
	".machine 0x0",
	".on_packet L0",
	".on_periodic L0",

	"L0:",
	"	ret",
	"	jump L1",
	"	jump L2",
	"L1:",
	"L2:",
	"	ret"
};

static Testcase labelsWithComments = {
	".version 0",
	".machine 0x0",
	".on_packet packet",
	".on_periodic periodic",

	"packet:",
	"periodic:",
	";",
	"	ret",
	";",
	"	ret"
};

static Testcase switchTableComments = {
	".version 0",
	".machine 0x0",
	".on_packet L0",
	".on_periodic L0",

	"L0:",
	"	switch {",
	"		; comment",
	"		0: L1",
	"	}",

	"L1:",
	"	ret"
};

static Testcase allVectorsUnique = {
	{ ".version 0" },
	{ ".machine 0x00000000000000000000000000000000" },
	{ "; machine id", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, true },
	{ "; version number", { 0x0 }, true },
	{ ".on_init L0", { 0x00, 0x07 } },
	{ ".on_packet L1", { 0x00, 0x08 } },
	{ ".on_periodic L2", { 0x00, 0x09 } },
	{ "" },
	{ "L0:" },
	{ "	ret", { 0x33 } },
	{ "L1:" },
	{ "	ret", { 0x33 } },
	{ "L2:" },
	{ "	ret", { 0x33 } }
};

template<typename It1, typename It2>
bool matches(It1& begin1, It1 end1, It2 begin2, It2 end2)
{
	while (begin2 != end2) {
		if (begin1 == end1 || *begin1 != *begin2)
			return false;

		++begin1;
		++begin2;
	}

	return true;
}

}

void checkAssembler(const std::vector<AssemblerLine> program)
{
	std::string text = "";
	std::vector<uint8_t> binary;

	for (const auto& line : program) {
		text += line.textual;
		text += "\n";
		std::copy(line.binary.begin(), line.binary.end(), std::back_inserter(binary));
	}

	auto parsed = hbt::ir::parse(hbt::util::MemoryBuffer(text));
	auto assembled = hbt::mc::assemble(*parsed);

	if (!binary.size())
		return;

	auto asIt = assembled.crange<uint8_t>().begin();
	auto asEnd = assembled.crange<uint8_t>().end();

	for (const auto& line : program) {
		auto asIt2 = asIt;
		bool lineMatch = matches(asIt2, asEnd, line.binary.begin(), line.binary.end());
		if (!lineMatch) {
			std::cout << "binary mismatch for line " << line.textual << "\n";
			std::string expected, got;
			for (unsigned i : line.binary) {
				expected += (boost::format(" %02x") % i).str();
				if (asIt == asEnd)
					got += " ??";
				else
					got += (boost::format(" %02x") % unsigned(*asIt++)).str();
			}
			std::cout << "expected" << expected << "\n";
			std::cout << "got     " << got << "\n";
		}
		asIt = asIt2;
		BOOST_REQUIRE(lineMatch);
	}
}

BOOST_AUTO_TEST_CASE(assemblerFull)
{
	checkAssembler(allFeatures);
}

BOOST_AUTO_TEST_CASE(assemblerNoInit)
{
	checkAssembler(noInit);
}

BOOST_AUTO_TEST_CASE(assemblerMultiLabel)
{
	checkAssembler(multiLabel);
}

BOOST_AUTO_TEST_CASE(assemblerLabelsWithComments)
{
	checkAssembler(labelsWithComments);
}

BOOST_AUTO_TEST_CASE(assemblerSwitchTableComments)
{
	checkAssembler(switchTableComments);
}

BOOST_AUTO_TEST_CASE(assemblerAllVectorsUnique)
{
	checkAssembler(allVectorsUnique);
}



void checkDisssembler(const std::vector<AssemblerLine> program)
{
	std::string text = "";
	std::vector<uint8_t> binary;

	for (const auto& line : program) {
		if (!line.ignoreTextOnDisassembly) {
			if (text.size())
				text += "\n";
			text += line.textual;
		}
		std::copy(line.binary.begin(), line.binary.end(), std::back_inserter(binary));
	}

	auto disassembled = hbt::mc::disassemble(hbt::util::MemoryBuffer(binary));
	auto printed = hbt::ir::prettyPrint(*disassembled);

	if (text != printed) {
		std::cout << "text\n" << text << "\n//--------\n";
		std::cout << "printed\n" << printed << "\n";
	}
	BOOST_CHECK(text == printed);
}

BOOST_AUTO_TEST_CASE(disassemblerFull)
{
	checkDisssembler(allFeatures);
}

BOOST_AUTO_TEST_CASE(disassemblerNoInit)
{
	checkDisssembler(noInit);
}

BOOST_AUTO_TEST_CASE(disassemblerAllVectorsUnique)
{
	checkDisssembler(allVectorsUnique);
}
