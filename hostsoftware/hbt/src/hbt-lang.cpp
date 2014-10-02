#include <iostream>
#include <sstream>

#include <unistd.h>

#include "Util/memorybuffer.hpp"
#include "Lang/parser.hpp"
#include "Lang/ast.hpp"
#include "Lang/astprinter.hpp"
#include "Lang/sema.hpp"

#include <boost/asio/ip/address_v6.hpp>
#include <boost/filesystem.hpp>

using namespace hbt::lang;

namespace {
typedef std::function<bool (std::unique_ptr<TranslationUnit>&)> pass_type;
}

static pass_type printAST(std::string into)
{
	if (into == "-") {
		return [] (std::unique_ptr<TranslationUnit>& tu) {
			ASTPrinter(std::cout).visit(*tu);
			return true;
		};
	} else {
		return [=] (std::unique_ptr<TranslationUnit>& tu) {
			std::ostringstream buf;

			ASTPrinter(buf).visit(*tu);
			hbt::util::MemoryBuffer(buf.str()).writeFile(into, true);
			return true;
		};
	}
}

static pass_type runSema()
{
	return [] (std::unique_ptr<TranslationUnit>& tu) {
		DiagnosticOutput diag(std::cout);
		SemanticVisitor(diag).visit(*tu);

		if (diag.warningCount() || diag.errorCount()) {
			std::cout << "\n";
			if (diag.errorCount())
				std::cout << "got " << diag.errorCount() << " errors";
			if (diag.warningCount()) {
				if (diag.errorCount())
					std::cout << " and ";
				else
					std::cout << "got ";
				std::cout << diag.warningCount() << " warnings";
			}
			std::cout << "\n";
		}
		return diag.errorCount() == 0;
	};
}

int main(int argc, char* argv[])
{
	static const char* helpMessage = R"(Usage: hbt-lang [options] [passes] <input>

Options:
  -help              print this message
  -I <dir>           add <dir> to list of include directories

Passes:
  -print-ast <file>  print current AST to <file>
  -sema              run semantic checks again (sema will always be run as
                     first non-printing pass)
)";

	if (argc <= 1) {
		std::cout << helpMessage;
		return 0;
	}

	const char* input = nullptr;

	std::vector<pass_type> passes;
	bool hadOnlyPrintPasses = true;

	std::vector<std::string> includePaths;

	auto getNextArg = [argc, argv] (int& arg) {
		if (arg + 1 >= argc) {
			std::cerr << "option " << argv[arg] << " needs an argument\n";
			exit(1);
		}
		arg += 1;
		return std::string(argv[arg]);
	};

	auto addNonSemaPass = [&passes, &hadOnlyPrintPasses] (pass_type pass, bool isPrint) {
		if (!isPrint && hadOnlyPrintPasses) {
			passes.push_back(runSema());
		}
		hadOnlyPrintPasses &= isPrint;
		passes.push_back(pass);
	};

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "-help") {
			std::cout << helpMessage;
			return 0;
		} else if (arg == "-I") {
			includePaths.push_back(getNextArg(i));
		} else if (arg == "-print-ast") {
			addNonSemaPass(printAST(getNextArg(i)), true);
		} else if (arg == "-sema") {
			passes.push_back(runSema());
		} else {
			if ((input && arg[0] == '-') || (!input && arg[0] == '-' && arg != "-")) {
				std::cerr << "superfluous argument '" << arg << "'\n";
				return 1;
			} else {
				input = argv[i];
			}
		}
	}

	if (hadOnlyPrintPasses)
		passes.push_back(runSema());

	if (!input) {
		std::cerr << "input missing\n";
		return 1;
	}

	auto buf = hbt::util::MemoryBuffer::loadFile(input, true);

	try {
		std::unique_ptr<char, void (*)(void*)> wd(get_current_dir_name(), free);
		boost::filesystem::path file(input);

		auto tu = hbt::lang::Parser(includePaths, 4).parse(file.is_absolute() ? file.native() : (wd.get() / file).native());

		for (auto& pass : passes) {
			if (!pass(tu))
				return 1;
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
