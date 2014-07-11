#include "IR/builder.hpp"
#include "IR/instruction.hpp"
#include "IR/parser.hpp"
#include "MC/assembler.hpp"

#include <iostream>
#include <fstream>
#include <map>
#include <tuple>

#include <boost/program_options.hpp>

static bool readStream(std::istream& stream, std::string& result)
{
	std::vector<char> buffer(65535);

	while (stream && !stream.eof()) {
		result.reserve(result.size() + buffer.size());

		stream.read(&buffer[0], buffer.size());
		int read = stream.gcount();

		if (!stream.bad())
			result.append(buffer.begin(), buffer.begin() + read);
	}

	return stream.eof();
}

static std::string readFile(const std::string& path)
{
	std::string result;
	bool success;

	if (path == "-") {
		success = readStream(std::cin, result);
	} else {
		std::ifstream file(path);

		if (!file)
			throw std::runtime_error("could not open file " + path);

		success = readStream(file, result);
	}

	if (!success)
		throw std::runtime_error("can't read file " + path);

	return result;
}

static bool writeStream(std::ostream& stream, const std::vector<uint8_t>& contents)
{
	stream.write((const char*) &contents[0], contents.size());

	return !stream.bad();
}

static void writeFile(const std::string& path, const std::vector<uint8_t>& contents)
{
	bool success;

	if (path == "-") {
		success = writeStream(std::cout, contents);
	} else {
		std::ofstream file(path);

		if (!file)
			throw std::runtime_error("could not open file " + path);

		success = writeStream(file, contents);
	}

	if (!success)
		throw std::runtime_error("can't write file " + path);
}

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

	if (!vm.count("input-file")) {
		std::cerr << "hbt-as: error: no input file specified\n";
		return 1;
	}

	if (!vm.count("output-file")) {
		output = "out.hbo";
	}

	std::string inputText;

	try {
		inputText = readFile(input);

		std::unique_ptr<hbt::ir::Program> program;

		try {
			program = hbt::ir::parse(inputText);
		} catch (const hbt::ir::ParseError& e) {
			std::cout << "hbt-as: error in input\n"
				<< "expected " << e.expected() << " at " << e.line() << ":" << e.column();
			if (e.detail().size())
				std::cout << " (" << e.detail() << ")";
			std::cout << "\n";
			return 1;
		}

		std::vector<uint8_t> assembly = hbt::mc::assemble(*program);

		writeFile(output, assembly);
	} catch (const std::exception& e) {
		std::cerr << "hbt-as: error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
