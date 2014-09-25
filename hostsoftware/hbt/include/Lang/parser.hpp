#ifndef INCLUDE_LANG_PARSER_HPP_986D1853A6D8E2D9
#define INCLUDE_LANG_PARSER_HPP_986D1853A6D8E2D9

#include <memory>
#include <string>
#include <vector>

#include "Lang/ast.hpp"

namespace hbt {

namespace util {
class MemoryBuffer;
}

namespace lang {

class ParseError : public std::runtime_error {
	private:
		std::vector<std::unique_ptr<std::string>> _strings;
		std::vector<std::unique_ptr<SourceLocation>> _locs;
		SourceLocation* _at;
		std::string _expected;
		std::string _got;

	public:
		ParseError(const SourceLocation& at, const std::string& expected, const std::string& got);

		const SourceLocation& at() const { return *_at; }
		const std::string& expected() const { return _expected; }
		const std::string& got() const { return _got; }
};

std::unique_ptr<TranslationUnit> parse(const util::MemoryBuffer& file, const std::string& fileName,
		const std::vector<std::string>& includePaths, int tabWidth = 4);

}
}

#endif
