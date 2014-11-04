#include "hbt/MC/builder.hpp"
#include "hbt/MC/disassembler.hpp"
#include "hbt/MC/instruction.hpp"
#include "hbt/MC/parser.hpp"
#include "hbt/MC/program.hpp"
#include "hbt/MC/program_printer.hpp"
#include "hbt/Util/memorybuffer.hpp"

#include <iostream>
#include <fstream>
#include <map>
#include <tuple>

#include <boost/program_options.hpp>

int main(int argc, char* argv[])
{
	namespace po = boost::program_options;

	std::string input, output;

	po::options_description visibleOpts(
			"Usage: hbt-dis [options] <inputs>\n"
			"\n"
			"Options");
	visibleOpts.add_options()
		("help,h", "produce help message")
		("version", "print version info")
		("output-file,o", po::value(&output), "output file");

	po::options_description allOpts;
	allOpts.add(visibleOpts).add_options()
		("input-file", po::value(&input));

	po::positional_options_description p;
	p.add("input-file", 1);

	po::variables_map vm;

	try {
		po::store(
			po::command_line_parser(argc, argv)
				.options(allOpts)
				.positional(p)
				.run(),
			vm);
		vm.notify();
	} catch (const std::exception& e) {
		std::cerr << "hbt-dis: error: " << e.what() << "\n";
		return 1;
	}

	if (vm.count("help")) {
		std::cout << visibleOpts;
		return 0;
	}

	if (!vm.count("input-file")) {
		std::cerr << "hbt-dis: error: no input file specified\n";
		return 1;
	}

	if (!vm.count("output-file")) {
		output = "out.hbs";
	}

	try {
		auto buffer = hbt::util::MemoryBuffer::loadFile(input, true);

		auto program = hbt::mc::disassemble(buffer);

		std::string disassembly = hbt::mc::prettyPrint(*program);

		hbt::util::MemoryBuffer(disassembly).writeFile(output, true);
	} catch (const std::exception& e) {
		std::cerr << "hbt-dis: error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
