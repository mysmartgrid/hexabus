#include <iomanip>
#include <iostream>
#include <sstream>

#include <unistd.h>

#include "Lang/ast.hpp"
#include "Lang/astprinter.hpp"
#include "Lang/codegen.hpp"
#include "Lang/parser.hpp"
#include "Lang/sema.hpp"
#include "MC/program.hpp"
#include "MC/program_printer.hpp"
#include "Util/memorybuffer.hpp"

#include <boost/asio/ip/address_v6.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

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

static pass_type runSemaExpect(const std::vector<std::pair<unsigned, std::string>>& patterns, const std::string& filePath)
{
	return [&patterns, &filePath] (std::unique_ptr<TranslationUnit>& tu) {
		std::stringstream buf;
		DiagnosticOutput diag(buf);
		SemanticVisitor(diag).visit(*tu);

		std::list<std::tuple<unsigned, boost::regex, const std::string&>> patternRegexes;

		for (const auto& pattern : patterns) {
			std::stringstream patternRegex;
			auto appendEscapedRegex = [&patternRegex] (const std::string& part) {
				for (char c : part)
					patternRegex << "\\x" << std::hex << std::setw(2) << unsigned((unsigned char) c);
			};

			auto expected = pattern.second;

			patternRegex << "^";
			if (expected.find("@<builtin>") == 0) {
				patternRegex << "<builtin>:0:0: .*?";
				expected = expected.substr(strlen("@<builtin>"));
			} else {
				appendEscapedRegex(filePath);
				patternRegex << ":" << std::dec << std::setw(0) << pattern.first << ":\\d+: .*?";
			}

			do {
				auto beginRe = expected.find("{{");
				if (beginRe == 0) {
					auto endPos = expected.find("}}");
					if (endPos == expected.npos) {
						patternRegex << expected;
						expected = "";
					} else {
						patternRegex << expected.substr(2, endPos - 2);
						expected = expected.substr(endPos + 2);
					}
				} else if (beginRe == expected.npos) {
					appendEscapedRegex(expected);
					expected = "";
				} else {
					appendEscapedRegex(expected.substr(0, beginRe));
					expected = expected.substr(beginRe);
				}
			} while (expected.size());

			patternRegex << ".*?$";

			patternRegexes.emplace_back(pattern.first, boost::regex(patternRegex.str()), pattern.second);
		}

		bool success = true;
		while (buf.tellg() != buf.tellp()) {
			std::string diagLine;
			while (diagLine.size() == 0 && !buf.fail()) {
				getline(buf, diagLine, '\n');
			}

			if (buf.fail())
				break;

			bool matched = false;
			for (auto it = patternRegexes.begin(), end = patternRegexes.end(); it != end; ++it) {
				if (boost::regex_match(diagLine, std::get<1>(*it))) {
					patternRegexes.erase(it);
					matched = true;
					break;
				}
			}

			success &= matched;
			if (!matched)
				std::cout << "unmatched diagnostic line: " << diagLine << "\n";
		}

		while (buf.peek() == '\n')
			buf.get();

		if (patternRegexes.size()) {
			for (auto& p : patternRegexes) {
				std::cout << "unmatched expectation in line " << std::get<0>(p) << ": " << std::get<2>(p) << "\n";
			}
			return false;
		}

		return success;
	};
}

static pass_type runCodegen(const std::string& file)
{
	return [file] (std::unique_ptr<TranslationUnit>& tu) {
		auto program = generateMachineCodeFor(*tu);
		std::string assembled = hbt::mc::prettyPrint(*program);
		hbt::util::MemoryBuffer(assembled).writeFile(file, true);
		return true;
	};
}

int main(int argc, char* argv[])
{
	static const char* helpMessage = R"(Usage: hbt-lang [options] [passes] <input>

Options:
  -help              print this message
  -I <dir>           add <dir> to list of include directories

Passes:
  -codegen <file>    run codegen and print output to <file>
  -print-ast <file>  print current AST to <file>
  -sema              run semantic checks again (sema will always be run as
                     first non-printing pass)
  -sema-expect       run semantic checks again, process #expect comments too
)";

	if (argc <= 1) {
		std::cout << helpMessage;
		return 0;
	}

	const char* input = nullptr;
	std::string fileName;

	std::vector<pass_type> passes;
	bool hadOnlyPrintPasses = true;

	std::vector<std::string> includePaths;

	std::vector<std::pair<unsigned, std::string>> semaExpected;

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
		} else if (arg == "-sema-expect") {
			passes.push_back(runSemaExpect(semaExpected, fileName));
			hadOnlyPrintPasses = false;
		} else if (arg == "-codegen") {
			addNonSemaPass(runCodegen(getNextArg(i)), false);
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

	try {
		std::unique_ptr<char, void (*)(void*)> wd(get_current_dir_name(), free);
		boost::filesystem::path file(input);

		Parser parser(includePaths, 4, &semaExpected);
		fileName = file.is_absolute() ? file.native() : (wd.get() / file).native();

		auto tu = parser.parse(fileName);

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
