#include "IR/builder.hpp"
#include "IR/instruction.hpp"
#include "IR/parser.hpp"
#include "IR/program.hpp"
#include "MC/assembler.hpp"
#include "Util/memorybuffer.hpp"

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
			"Usage: hbt-as [options] <inputs>\n"
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
		std::cerr << "hbt-as: error: " << e.what() << "\n";
		return 1;
	}

	if (vm.count("help")) {
		std::cout << visibleOpts;
		return 0;
	}

	if (!vm.count("input-file")) {
		std::cerr << "hbt-as: error: no input file specified\n";
		return 1;
	}

	if (!vm.count("output-file")) {
		output = "out.hbo";
	}

	try {
		auto buffer = hbt::util::MemoryBuffer::loadFile(input);

		std::unique_ptr<hbt::ir::Program> program = hbt::ir::parse(buffer);

		hbt::util::MemoryBuffer assembly;

		assembly = std::move(hbt::mc::assemble(*program));

		assembly.writeFile(output);
	} catch (const hbt::ir::ParseError& e) {
		std::cout << "hbt-as: error in input\n"
			<< "expected " << e.expected() << " at " << e.line() << ":" << e.column();
		if (e.detail().size())
			std::cout << " (" << e.detail() << ")";
		std::cout << "\n";
		return 1;
	} catch (const hbt::ir::InvalidProgram& e) {
		std::cout << "hbt-as: invalid program\n"
			<< e.what() << "\n";
		if (e.extra().size())
			std::cout << e.extra() << "\n";
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "hbt-as: error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
