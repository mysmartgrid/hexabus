#include "IR/builder.hpp"
#include "IR/instruction.hpp"
#include "IR/parser.hpp"
#include "IR/program_printer.hpp"
#include "MC/assembler.hpp"
#include "MC/disassembler.hpp"

#include <iostream>
#include <fstream>
#include <map>
#include <tuple>

int main()
{
	std::ifstream t("test.hbs");

	std::string input((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());

	try {
		auto p = hbt::ir::parse(input);
		std::string program = hbt::ir::prettyPrint(*p);

		std::cout << "program looks like:\n" << program << "\n\n";

		program = hbt::ir::prettyPrint(*hbt::ir::parse(program));
		std::cout << "program looks like:\n" << program << "\n\n";

		auto raw = hbt::mc::assemble(*p);
		for (unsigned i : raw)
			printf("%02x ", i);

		p = hbt::mc::disassemble(raw, true);
		program = hbt::ir::prettyPrint(*p);
		std::cout << "program looks like:\n" << program << "\n\n";
	} catch (const hbt::ir::ParseError& pe) {
		std::cout << "expected " << pe.expected() << " at "
			<< pe.line() << ":" << pe.column() << std::endl;
		if (pe.detail().size())
			std::cout << " ==> " << pe.detail() << std::endl;
	} catch (const hbt::ir::InvalidProgram& ip) {
		std::cout << ip.what() << std::endl;
	}
}
