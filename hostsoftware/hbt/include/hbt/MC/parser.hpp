#ifndef INCLUDE_MC_PARSER_HPP_E6C996D913035D86
#define INCLUDE_MC_PARSER_HPP_E6C996D913035D86

#include <memory>
#include <string>

namespace hbt {

namespace util {
class MemoryBuffer;
}

namespace mc {

class Program;

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

std::unique_ptr<Program> parse(const hbt::util::MemoryBuffer& input);

}
}

#endif
