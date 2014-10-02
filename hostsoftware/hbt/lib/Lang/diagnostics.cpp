#include "Lang/diagnostics.hpp"

#include "Lang/ast.hpp"

namespace hbt {
namespace lang {

static void printIncludeStack(std::ostream& out, const SourceLocation* sloc)
{
	if (!sloc)
		return;

	printIncludeStack(out, sloc->parent());
	out << "in file included from " << sloc->file() << ":" << sloc->line() << ":" << sloc->col() << ":\n";
}

void DiagnosticOutput::printFirst(const Diagnostic& diag)
{
	printIncludeStack(out, diag.sloc->parent());
	printRest(diag);
}

void DiagnosticOutput::printRest(const Diagnostic& diag)
{
	out << diag.sloc->file() << ":" << diag.sloc->line() << ":" << diag.sloc->col() << ": ";

	switch (diag.kind) {
	case DiagnosticKind::Hint: out << "hint: "; break;
	case DiagnosticKind::Warning: out << "warning: "; break;
	case DiagnosticKind::Error: out << "error: "; break;
	default: throw "invalid diag kind";
	}

	out << diag.message << "\n";

	if (diag.kind == DiagnosticKind::Error)
		_errorCount++;
	if (diag.kind == DiagnosticKind::Warning)
		_warningCount++;
}

void DiagnosticOutput::printHints()
{
	for (const auto& diag : hintStack) {
		if (diag.sloc)
			printRest(diag);
		else
			out << "hint: " << diag.message << "\n";
	}
}

DiagnosticOutput::HintHandle DiagnosticOutput::useHint(const SourceLocation* sloc, std::string message)
{
	hintStack.push_front({ DiagnosticKind::Hint, sloc, std::move(message) });
	return { hintStack };
}

}
}
