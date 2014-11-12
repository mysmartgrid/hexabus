#ifndef INCLUDE_LANG_SEMA_SCOPE_HPP_7D8E5E07FFCCDA5B
#define INCLUDE_LANG_SEMA_SCOPE_HPP_7D8E5E07FFCCDA5B

#include <map>
#include <memory>
#include <vector>

#include "hbt/Lang/ast.hpp"

namespace hbt {
namespace lang {

class BuiltinFunction : public Declaration {
private:
	static std::string builtinFile;
	static SourceLocation builtinSloc;

	static BuiltinFunction _now;
	static BuiltinFunction _second;
	static BuiltinFunction _minute;
	static BuiltinFunction _hour;
	static BuiltinFunction _day;
	static BuiltinFunction _month;
	static BuiltinFunction _year;
	static BuiltinFunction _weekday;

	std::string _name;
	Type _returnType;
	std::vector<Type> _argumentTypes;

public:
	BuiltinFunction(std::string name, Type retType, std::vector<Type> argTypes)
		: _name(std::move(name)), _returnType(retType), _argumentTypes(std::move(argTypes))
	{}

	const SourceLocation& sloc() const override { return builtinSloc; }
	const std::string& identifier() const { return _name; }
	Type returnType() const { return _returnType; }
	const std::vector<Type>& argumentTypes() const { return _argumentTypes; }

	static BuiltinFunction* now() { return &_now; }
	static BuiltinFunction* second() { return &_second; }
	static BuiltinFunction* minute() { return &_minute; }
	static BuiltinFunction* hour() { return &_hour; }
	static BuiltinFunction* day() { return &_day; }
	static BuiltinFunction* month() { return &_month; }
	static BuiltinFunction* year() { return &_year; }
	static BuiltinFunction* weekday() { return &_weekday; }
};


class Scope {
private:
	Scope* _parent;
	std::map<std::string, Declaration*> _entries;

public:
	Scope(Scope* parent = nullptr)
		: _parent(parent)
	{}

	Scope* parent() { return _parent; }

	Declaration* resolve(const std::string& name);

	std::pair<Declaration*, bool> insert(Declaration& decl);
	std::pair<Declaration*, bool> insert(const std::string& name, Declaration& decl);
};

}
}

#endif
