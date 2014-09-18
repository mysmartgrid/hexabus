#ifndef INCLUDE_LANG_PARSER_HPP_986D1853A6D8E2D9
#define INCLUDE_LANG_PARSER_HPP_986D1853A6D8E2D9

#include <memory>

namespace hbt {

namespace util {
class MemoryBuffer;
}

namespace lang {

class TranslationUnit;

class ParseError : public std::runtime_error {
	private:
		int _line, _column;
		std::string _expected;
		std::string _detail;

	public:
		ParseError(int line, int column, const std::string& expected, const std::string& detail)
			: runtime_error(expected),
			  _line(line), _column(column), _expected(expected), _detail(detail)
		{}

		int line() const { return _line; }
		int column() const { return _column; }
		const std::string& expected() const { return _expected; }
		const std::string& detail() const { return _detail; }
};

std::unique_ptr<TranslationUnit> parse(const hbt::util::MemoryBuffer& input);

}
}

#endif
