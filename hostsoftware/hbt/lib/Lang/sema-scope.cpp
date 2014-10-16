#include "Lang/sema-scope.hpp"

namespace hbt {
namespace lang {

std::string BuiltinFunction::builtinFile("<builtin>");
SourceLocation BuiltinFunction::builtinSloc(&builtinFile, 0, 0);



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
