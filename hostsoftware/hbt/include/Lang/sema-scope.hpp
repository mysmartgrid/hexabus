#ifndef INCLUDE_LANG_SEMA_SCOPE_HPP_7D8E5E07FFCCDA5B
#define INCLUDE_LANG_SEMA_SCOPE_HPP_7D8E5E07FFCCDA5B

#include <map>
#include <memory>

#include "Lang/ast.hpp"

namespace hbt {
namespace lang {

class BuiltinFunction : public Declaration {
private:
	static std::string builtinFile;
	static SourceLocation builtinSloc;

	std::string _name;

public:
	BuiltinFunction(std::string name)
		: _name(std::move(name))
	{}

	const SourceLocation& sloc() const override { return builtinSloc; }
	const std::string& identifier() const { return _name; }
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
};

}
}

#endif
