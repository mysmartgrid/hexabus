#include "hbt/Lang/sema-scope.hpp"

namespace hbt {
namespace lang {

std::string BuiltinFunction::builtinFile("<builtin>");
SourceLocation BuiltinFunction::builtinSloc(&builtinFile, 0, 0);

BuiltinFunction BuiltinFunction::_now("now", Type::Int64, {});
BuiltinFunction BuiltinFunction::_second("second", Type::Int32, { Type::Int64 });
BuiltinFunction BuiltinFunction::_minute("minute", Type::Int32, { Type::Int64 });
BuiltinFunction BuiltinFunction::_hour("hour", Type::Int32, { Type::Int64 });
BuiltinFunction BuiltinFunction::_day("day", Type::Int32, { Type::Int64 });
BuiltinFunction BuiltinFunction::_month("month", Type::Int32, { Type::Int64 });
BuiltinFunction BuiltinFunction::_year("year", Type::Int32, { Type::Int64 });
BuiltinFunction BuiltinFunction::_weekday("weekday", Type::Int32, { Type::Int64 });


Declaration* Scope::resolve(const std::string& name)
{
	auto it = _entries.find(name);
	if (it != _entries.end())
		return it->second;
	else if (_parent)
		return _parent->resolve(name);
	else
		return nullptr;
}

std::pair<Declaration*, bool> Scope::insert(Declaration& decl)
{
	auto res = _entries.insert({ decl.identifier(), &decl });
	return { res.first->second, res.second };
}

}
}
