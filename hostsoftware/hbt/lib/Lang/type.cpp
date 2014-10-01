#include "Lang/type.hpp"

namespace hbt {
namespace lang {

const char* typeName(Type type)
{
	switch (type) {
	case Type::Unknown: return "<""??"">"; break;
	case Type::Bool:    return "bool"; break;
	case Type::UInt8:   return "uint8"; break;
	case Type::UInt32:  return "uint32"; break;
	case Type::UInt64:  return "uint64"; break;
	case Type::Float:   return "float"; break;
	default: throw "unknown type";
	}
}

Type commonType(Type a, Type b)
{
	if (a == Type::Unknown || b == Type::Unknown)
		return Type::Unknown;

	switch (a) {
	case Type::Unknown:
		return a;

	case Type::Bool:
	case Type::UInt8:
	case Type::UInt32:
		if (b == Type::UInt64)
			return Type::UInt64;
		else if (b == Type::Float)
			return Type::Float;
		else
			return Type::UInt32;

	case Type::UInt64:
		if (b == Type::Float)
			return Type::Float;
		else
			return Type::UInt64;

	case Type::Float:
		return Type::Float;
	}
}

bool isAssignableFrom(Type to, Type from)
{
	switch (to) {
	case Type::Bool:
	case Type::UInt8:
	case Type::Float:
		return from == to;

	case Type::UInt32:
		return from == to || from == Type::UInt8;

	case Type::UInt64:
		return from == to || from == Type::UInt32 || from == Type::UInt8;

	default:
		return false;
	}
}

bool isIntType(Type t)
{
	return isAssignableFrom(Type::UInt64, t);
}

}
}
