#ifndef INCLUDE_LANG_TYPE_HPP_0FEBFB7EE685071D
#define INCLUDE_LANG_TYPE_HPP_0FEBFB7EE685071D

namespace hbt {
namespace lang {

enum class Type {
	Unknown,

	Bool,
	UInt8,
	UInt32,
	UInt64,
	Float,
};

const char* typeName(Type type);

Type commonType(Type a, Type b);

bool isAssignableFrom(Type to, Type from);

inline bool isIntType(Type t)
{
	switch (t) {
	case Type::UInt8:
	case Type::UInt32:
	case Type::UInt64:
		return true;

	default:
		return false;
	}
}

inline bool isConstexprType(Type t)
{
	return t == Type::Bool || isIntType(t);
}

}
}

#endif
