#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "hbt/IR/codegen.hpp"
#include "hbt/IR/module.hpp"
#include "hbt/IR/moduleprinter.hpp"
#include "hbt/Lang/ast.hpp"
#include "hbt/Lang/astprinter.hpp"
#include "hbt/Lang/codegen.hpp"
#include "hbt/Lang/parser.hpp"
#include "hbt/Lang/sema.hpp"
#include "hbt/MC/assembler.hpp"
#include "hbt/MC/builder.hpp"
#include "hbt/MC/opt.hpp"
#include "hbt/MC/program.hpp"
#include "hbt/MC/program_printer.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

using namespace hbt::lang;

namespace {

enum class TargetFormat {
	AST,
	IR,
	MC,
	Object
};

class Driver {
private:
	std::string input;
	std::string output = "%d";
	bool syntaxOnly = false;
	bool warningsAreErrors = false;
	unsigned tabWidth = 4;
	TargetFormat targetFormat = TargetFormat::Object;
	std::vector<std::string> includePaths;
	std::set<std::string> devices;
	std::map<const Device*, std::unique_ptr<hbt::ir::Module>> deviceModules;
	std::map<const Device*, std::unique_ptr<hbt::mc::Program>> devicePrograms;

	std::vector<std::function<void (hbt::mc::Builder&)>> mcOptimizers;

	void printHelp();

	void parseArgs(int argc, char* argv[]);

	std::string fileFor(const Device* dev);
	void saveDeviceBuffer(const hbt::util::MemoryBuffer& buf, const Device* dev);

	bool ensureAllDeviceNamesAreValid(TranslationUnit& tu);

	std::unique_ptr<TranslationUnit> parse(std::vector<std::pair<unsigned, std::string>>* semaExpected = nullptr);
	int runSyntaxOnly();
	int runIRGen(TranslationUnit& tu);
	int runMCGen();

public:
	int run(int argc, char* argv[]);
};

void Driver::printHelp()
{
	std::cout <<
R"(Usage: hbc [options] <input> [devices]

Options:
  -help             print this message
  -I <dir>          add <dir> to list of include directories
  -fout <fmt>       sets output format (default: o)
    ast               emit processed AST
    ir                emit intermediate code
    mc                emit assembler code
    o                 emit complete object files
  -fsyntax-only     only check syntax of input
  -ftab-width       assumed width of tabs (default: 4)
  -o <pattern>      output files to <pattern> (default: %d)
                      <pattern> will be used to generate file names. Directories
                      used in <pattern> must exist. %d will be replaced by the
                      name of the device the file belongs to, %% yields a %.
  -OM<arg>          configure MC optimizers
    -                 clear optimizer list
    jumpthreading
    rm-unreachable
    codemotion        replace jumps with targets of jumps, if possible (implies
                      jumpthreading, rm-unreachable)
  -Werror           turn warnings into errors
)";
}

void Driver::parseArgs(int argc, char* argv[])
{
	if (argc <= 1) {
		printHelp();
		exit(1);
	}

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		auto getNextArg = [argc, argv, &i] () {
			if (i + 1 >= argc) {
				std::cerr << "option " << argv[i] << " needs an argument\n";
				exit(1);
			}
			i += 1;
			return std::string(argv[i]);
		};

		if (arg == "-help") {
			printHelp();
			exit(0);
		} else if (arg == "-I") {
			includePaths.push_back(getNextArg());
		} else if (arg == "-fout") {
			auto out = getNextArg();
			if (out == "ast") {
				targetFormat = TargetFormat::AST;
			} else if (out == "ir") {
				targetFormat = TargetFormat::IR;
			} else if (out == "mc") {
				targetFormat = TargetFormat::MC;
			} else if (out == "o") {
				targetFormat = TargetFormat::Object;
			} else {
				std::cerr << "invalid argument " << out << " for -fout\n";
				exit(1);
			}
		} else if (arg == "-fsyntax-only") {
			syntaxOnly = true;
		} else if (arg.substr(0, 3) == "-OM") {
			if (arg == "-OM-") {
				mcOptimizers.clear();
			} else if (arg == "-OMjumpthreading") {
				mcOptimizers.push_back(hbt::mc::threadTrivialJumps);
			} else if (arg == "-OMrm-unreachable") {
				mcOptimizers.push_back(hbt::mc::pruneUnreachableInstructions);
			} else if (arg == "-OMcodemotion") {
				mcOptimizers.push_back(hbt::mc::moveJumpTargets);
			} else {
				std::cerr << "invalid argument " << arg.substr(2) << " for -OM\n";
				exit(1);
			}
		} else if (arg == "-o") {
			output = getNextArg();
		} else if (arg == "-Werror") {
			warningsAreErrors = true;
		} else {
			if ((!input.empty() && arg[0] == '-') || (input.empty() && arg[0] == '-' && arg != "-")) {
				std::cerr << "superfluous argument '" << arg << "'\n";
				exit(1);
			} else if (input.empty()) {
				std::unique_ptr<char, void (*)(void*)> wd(get_current_dir_name(), free);
				boost::filesystem::path file(arg);

				input = file.is_absolute() ? file.native() : (wd.get() / file).native();
			} else {
				devices.insert(arg);
			}
		}
	}
}

std::string Driver::fileFor(const Device* dev)
{
	std::stringstream out;

	bool inFormat = false;
	for (char c : output) {
		if (inFormat) {
			switch (c) {
			case '%': out << '%'; break;
			case 'd': out << dev->name().name(); break;
			default:
				std::cerr << "invalid output format\n";
				exit(1);
			}
			inFormat = false;
			continue;
		}

		if (c == '%') {
			inFormat = true;
			continue;
		}

		out << c;
	}

	return out.str();
}

void Driver::saveDeviceBuffer(const hbt::util::MemoryBuffer& buf, const Device* dev)
{
	try {
		if (dev)
			buf.writeFile(fileFor(dev), true);
		else
			buf.writeFile(output, true);
	} catch (const std::runtime_error& re) {
		std::cerr << "hbc: error: " << re.what() << '\n';
		exit(1);
	}
}

bool Driver::ensureAllDeviceNamesAreValid(TranslationUnit& tu)
{
	std::set<std::string> availableDevices;

	for (auto& part : tu.items()) {
		if (auto dev = dynamic_cast<const Device*>(part.get())) {
			availableDevices.insert(dev->name().name());
		}
	}

	bool valid = true;
	for (auto& dev : devices) {
		if (!availableDevices.count(dev)) {
			std::cout << "unknown device " << dev << "\n";
			valid = true;
		}
	}

	return valid;
}

std::unique_ptr<TranslationUnit> Driver::parse(std::vector<std::pair<unsigned, std::string>>* semaExpected)
{
	Parser parser(includePaths, 4, semaExpected);

	try {
		return parser.parse(input);
	} catch (const ParseError& e) {
		const auto* sloc = &e.at();
		std::cout << sloc->file() << ":" << sloc->line() << ":" << sloc->col()
			<< " error: expected " << e.expected() << ", got " << e.got() << "\n";
		while ((sloc = sloc->parent())) {
			std::cout << "included from " << sloc->file() << ":" << sloc->line() << ":" << sloc->col() << "\n";
		}
		exit(1);
	}
}

int Driver::runSyntaxOnly()
{
	std::vector<std::pair<unsigned, std::string>> expected;

	auto tu = parse(&expected);

	std::stringstream buf;
	DiagnosticOutput diag(buf);
	SemanticVisitor(diag).visit(*tu);

	std::list<std::tuple<unsigned, boost::regex, const std::string&>> patternRegexes;

	for (const auto& pattern : expected) {
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
			appendEscapedRegex(input);
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
		return 1;
	}

	return success ? 0 : 1;
}

int Driver::runIRGen(TranslationUnit& tu)
{
	auto machines = collectMachines(tu);

	for (auto& machine : machines) {
		if (!devices.empty() && !devices.count(machine.first->name().name()))
			continue;

		auto irb = generateIR(machine.first, machine.second);

		std::string verifierMsg;
		auto irm = irb->finish(&verifierMsg);

		if (!irm) {
			std::cerr
				<< "MODULE FAILED VERIFICATION\n\n"
				<< verifierMsg
				<< "\n\nModule text:\n\n"
				<< hbt::ir::prettyPrint(*irb) << '\n';
			return 1;
		}

		deviceModules.emplace(machine.first, std::move(irm));
	}

	return 0;
}

int Driver::runMCGen()
{
	for (auto& module : deviceModules) {
		auto mcb = hbt::ir::generateMachineCode(*module.second);

		for (auto& opt : mcOptimizers)
			opt(*mcb);

		devicePrograms.emplace(module.first, mcb->finish());
	}

	return 0;
}

int Driver::run(int argc, char* argv[])
{
	mcOptimizers.push_back(hbt::mc::moveJumpTargets);

	parseArgs(argc, argv);

	if (input.empty()) {
		std::cerr << "input file missing\n";
		return 1;
	}

	if (syntaxOnly)
		return runSyntaxOnly();

	auto tu = parse();

	DiagnosticOutput diag(std::cout);
	SemanticVisitor(diag).visit(*tu);

	if ((diag.warningCount() && warningsAreErrors) || diag.errorCount()) {
		std::cout << "\n";
		if (diag.errorCount())
			std::cout << "got " << diag.errorCount() << " errors";
		if (diag.warningCount()) {
			if (diag.errorCount())
				std::cout << " and ";
			else
				std::cout << "got ";
			std::cout << diag.warningCount() << " warnings";
			if (warningsAreErrors)
				std::cout << " (treated as errors)";
		}
		std::cout << "\n";
		return 1;
	}

	if (targetFormat == TargetFormat::AST) {
		std::stringstream buf;

		ASTPrinter(buf).visit(*tu);
		saveDeviceBuffer(hbt::util::MemoryBuffer(buf.str()), nullptr);
		return 0;
	}

	if (!ensureAllDeviceNamesAreValid(*tu))
		return 1;

	if (int res = runIRGen(*tu))
		return res;

	if (targetFormat == TargetFormat::IR) {
		for (auto& module : deviceModules) {
			std::stringstream out;

			out << "; IR for device " << module.first->name().name() << "\n\n";
			out << hbt::ir::prettyPrint(*module.second);
			if (output == "-")
				out << "\n\n";
			saveDeviceBuffer(hbt::util::MemoryBuffer(out.str()), module.first);
		}
		return 0;
	}

	if (int res = runMCGen())
		return res;

	if (targetFormat == TargetFormat::MC) {
		for (auto& program : devicePrograms) {
			std::stringstream out;

			out << "; Program for device " << program.first->name().name() << "\n\n";
			out << hbt::mc::prettyPrint(*program.second);
			if (output == "-")
				out << "\n\n";
			saveDeviceBuffer(hbt::util::MemoryBuffer(out.str()), program.first);
		}
		return 0;
	}

	if (targetFormat == TargetFormat::Object) {
		for (auto& program : devicePrograms)
			saveDeviceBuffer(hbt::mc::assemble(*program.second), program.first);

		return 0;
	}

	return 1;
}

}

int main(int argc, char* argv[])
{
	Driver driver;

	return driver.run(argc, argv);
}
