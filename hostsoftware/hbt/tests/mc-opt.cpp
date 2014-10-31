#define BOOST_TEST_MODULE MC optimization tests
#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <vector>

#include "MC/builder.hpp"
#include "MC/opt.hpp"
#include "MC/parser.hpp"
#include "MC/program.hpp"
#include "MC/program_printer.hpp"
#include "Util/memorybuffer.hpp"

namespace {

struct TestLine {
	const char* before;
	const char* after;
};

static const char empty[1] = { 0 };

struct Testcase {
	std::vector<TestLine> lines;

	Testcase(std::initializer_list<TestLine> lines)
		: lines(lines)
	{}

	std::string before() const
	{
		std::string result;
		for (auto& line : lines) {
			if (line.before && line.before != empty)
				result = result + line.before + '\n';
		}
		return result.substr(0, result.size() - 1);
	}

	std::string after() const
	{
		std::string result;
		for (auto& line : lines) {
			if (line.after && line.after != empty)
				result = result + line.after + '\n';
			else if (!line.after)
				result = result + line.before + '\n';
		}
		return result.substr(0, result.size() - 1);
	}
};

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

static Testcase transitiveJumps = {
	{ ".version 0" },
	{ ".on_init L2" },
	{ ".on_packet L1", ".on_packet L2" },
	{ "" },
	{ "	switch8 {" },
	{ "		0: L0", "		0: L2" },
	{ "	}" },
	{ "L0:" },
	{ "	jump L2" },
	{ "L1:" },
	{ "	jump L2" },
	{ "L2:" },
	{ "	ret" },
};

static Testcase unreachableSeqs = {
	{ ".version 0" },
	{ ".on_init L1" },
	{ ".on_packet L0", },
	{ "" },
	{ "	switch8 {",  empty },
	{ "		0: L0", empty },
	{ "	}",         empty },
	{ "L0:" },
	{ "	jump L1" },
	{ "	ret",       empty },
	{ "L1:" },
	{ "	ret" },
	{ "	ret",       empty },
};

static Testcase moveableJumpTargets = {
	{ ".version 0" },
	{ ".on_init L1" },
	{ ".on_packet L0", },
	{ "" },
	{ "L0:" },
	{ "	pop" },
	{ "	jump L2", empty },
	{ empty,      "L2:" },
	{ empty,      "	pop" },
	{ empty,      "L3:" },
	{ empty,      "	ret" },
	{ "L1:" },
	{ "	ret" },
	{ "L2:",      empty },
	{ "	pop",     empty },
	{ "	jump L3", empty },
	{ "L3:"     , empty },
	{ "	ret",     empty },
};

static Testcase noInvalidMoves = {
	{ ".version 0" },
	{ ".on_init L1", },
	{ ".on_packet L0", },
	{ "" },
	{ "L0:" },
	{ "	pop" },
	{ "	jump L2" },
	{ "L1:" },
	{ "	pop" },
	{ "	jump L3" },
	{ "L2:" },
	{ "	pop" },
	{ "L3:" },
	{ "	ret" },
};

}

static void testTransform(const Testcase& tc, void (&fn)(hbt::mc::Builder&))
{
	auto parsed = hbt::mc::parse(hbt::util::MemoryBuffer(tc.before()));
	hbt::mc::Builder builder(std::move(*parsed));

	fn(builder);

	auto program = builder.finish();

	auto wanted = tc.after();
	auto after = hbt::mc::prettyPrint(*program);

	if (wanted != after) {
		std::cout << "got\n" << after << "\n//--------------\n"
			<< "expected\n" << wanted << "\n";
	}
	BOOST_CHECK(wanted == after);
}

BOOST_AUTO_TEST_CASE(opt_threadTrivialJumps)
{
	testTransform(transitiveJumps, hbt::mc::threadTrivialJumps);
}

BOOST_AUTO_TEST_CASE(opt_pruneUnreachableInstructions)
{
	testTransform(unreachableSeqs, hbt::mc::pruneUnreachableInstructions);
}

BOOST_AUTO_TEST_CASE(opt_moveJumpTargets)
{
	testTransform(moveableJumpTargets, hbt::mc::moveJumpTargets);
	testTransform(noInvalidMoves, hbt::mc::moveJumpTargets);
}
