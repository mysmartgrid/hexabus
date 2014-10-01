#ifndef INCLUDE_LANG_DIAGNOSTICS_HPP_E8A4759B1FEB00EB
#define INCLUDE_LANG_DIAGNOSTICS_HPP_E8A4759B1FEB00EB

#include <list>
#include <ostream>
#include <string>

namespace hbt {
namespace lang {

class SourceLocation;

enum class DiagnosticKind {
	Hint,
	Error,
};

struct Diagnostic {
	DiagnosticKind kind;
	const SourceLocation* sloc;
	std::string message;
};

class DiagnosticOutput {
	class HintHandle {
		friend class DiagnosticOutput;

	private:
		std::list<Diagnostic>* list;

		HintHandle(std::list<Diagnostic>& list)
			: list(&list)
		{}

	public:
		HintHandle(HintHandle&& other)
			: list(other.list)
		{
			other.list = nullptr;
		}

		~HintHandle()
		{
			if (list)
				list->pop_front();
		}
	};

private:
	std::ostream& out;
	std::list<Diagnostic> hintStack;
	bool addExtraNewlines;
	unsigned _errorCount;

	void printFirst(const Diagnostic& diag);
	void printRest(const Diagnostic& diag);
	void printRest() {}
	void printHints();

	template<typename... Diags>
	void printRest(const Diagnostic& diag, const Diags&... rest)
	{
		printRest(diag);
		printRest(rest...);
	}

public:
	DiagnosticOutput(std::ostream& out)
		: out(out), hintStack(), addExtraNewlines(false), _errorCount(0)
	{}

	DiagnosticOutput(const DiagnosticOutput&) = delete;
	DiagnosticOutput& operator=(const DiagnosticOutput&) = delete;

	template<typename... Diags>
	void print(const Diagnostic& diag, const Diags&... rest)
	{
		if (addExtraNewlines)
			out << "\n";

		printFirst(diag);
		printRest(rest...);
		printHints();
		addExtraNewlines = true;
	}

	HintHandle useHint(const SourceLocation* sloc, std::string message);
	HintHandle useHint(const SourceLocation& sloc, std::string message)
	{
		return useHint(&sloc, std::move(message));
	}

	unsigned errorCount() const { return _errorCount; }
};

}
}

#endif
