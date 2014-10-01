#include <iostream>

#include <unistd.h>

#include "Util/memorybuffer.hpp"
#include "Lang/parser.hpp"
#include "Lang/ast.hpp"
#include "Lang/astprinter.hpp"
#include "Lang/sema.hpp"

#include <boost/asio/ip/address_v6.hpp>
#include <boost/filesystem.hpp>

using namespace hbt::lang;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "input missing\n";
		return 1;
	}

	auto buf = hbt::util::MemoryBuffer::loadFile(argv[1], true);

	try {
		std::unique_ptr<char, void (*)(void*)> wd(get_current_dir_name(), free);
		boost::filesystem::path file(argv[1]);

		auto tu = hbt::lang::Parser({ wd.get() }, 4).parse(file.is_absolute() ? file.native() : (wd.get() / file).native());

		ASTPrinter(std::cout).visit(*tu);

		DiagnosticOutput diag(std::cout);
		SemanticVisitor(diag).visit(*tu);

		if (diag.errorCount()) {
			std::cout << "\n\n" << "got " << diag.errorCount() << " errors\n";
		}
	} catch (const hbt::lang::ParseError& e) {
		const auto* sloc = &e.at();
		std::cout << sloc->file() << ":" << sloc->line() << ":" << sloc->col()
			<< " error: expected " << e.expected() << ", got " << e.got() << "\n";
		while ((sloc = sloc->parent())) {
			std::cout << "included from " << sloc->file() << ":" << sloc->line() << ":" << sloc->col() << "\n";
		}
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "hbt-lang: error: " << e.what() << "\n";
		return 1;
	}
}
