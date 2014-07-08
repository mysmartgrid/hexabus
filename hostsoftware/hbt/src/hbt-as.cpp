#include "IR/builder.hpp"
#include "IR/instruction.hpp"
#include "IR/parser.hpp"

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
		hbt::ir::parse(input);
	} catch (const hbt::ir::ParseError& pe) {
		std::cout << "expected " << pe.expected() << " at "
			<< pe.line() << ":" << pe.column() << std::endl;
		if (pe.detail().size())
			std::cout << " ==> " << pe.detail() << std::endl;
	}
}
