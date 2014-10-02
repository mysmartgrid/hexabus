#ifndef INCLUDE_LANG_PARSER_HPP_986D1853A6D8E2D9
#define INCLUDE_LANG_PARSER_HPP_986D1853A6D8E2D9

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Lang/ast.hpp"
#include "Util/memorybuffer.hpp"

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

class Parser {
	private:
		struct FileData {
			util::MemoryBuffer buf;
			std::string fullPath;
		};

		std::vector<std::string> _includePaths;
		int _tabWidth;
		std::map<unsigned, std::string>* _expectations;

		std::set<std::string> _filesAlreadyIncluded;
		std::set<std::string> _currentIncludeStack;

		FileData loadFile(const std::string& file, const std::string* extraSearchDir);
		std::list<std::unique_ptr<ProgramPart>> parseFile(const FileData& fileData, const std::string* pathPtr,
				const SourceLocation* includedFrom = nullptr);
		std::list<std::unique_ptr<ProgramPart>> parseRecursive(IncludeLine& include, const std::string* extraSearchDir);

	public:
		Parser(std::vector<std::string> includePaths, int tabWidth = 4, std::map<unsigned, std::string>* expectations = nullptr);

		std::unique_ptr<TranslationUnit> parse(const std::string& fileName);
};

}
}

#endif
