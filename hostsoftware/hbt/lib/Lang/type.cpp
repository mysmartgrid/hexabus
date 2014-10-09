#include "Lang/type.hpp"

#include <algorithm>

namespace hbt {
namespace lang {

const char* typeName(Type type)
{
	switch (type) {
	case Type::Unknown: return "<""??"">"; break;
	case Type::Bool:    return "bool"; break;
	case Type::UInt8:   return "uint8"; break;
	case Type::UInt16:  return "uint16"; break;
	case Type::UInt32:  return "uint32"; break;
	case Type::UInt64:  return "uint64"; break;
	case Type::Int8:    return "int8"; break;
	case Type::Int16:   return "int16"; break;
	case Type::Int32:   return "int32"; break;
	case Type::Int64:   return "int64"; break;
	case Type::Float:   return "float"; break;
	default: throw "unknown type";
	}
}

static unsigned rank(Type t)
{
	switch (t) {
	case Type::Bool:
		return 0;

	case Type::UInt8:
	case Type::Int8:
		return 1;

	case Type::UInt16:
	case Type::Int16:
		return 2;

	case Type::UInt32:
	case Type::Int32:
		return 3;

	case Type::UInt64:
	case Type::Int64:
		return 4;

	default: throw "unknown type";
	}
}

Type promote(Type t)
{
	switch (t) {
	case Type::UInt8:
	case Type::UInt16:
	case Type::Int8:
	case Type::Int16:
	case Type::Int32:
		return Type::Int32;

	default:
		return t;
	}
}

Type commonType(Type a, Type b)
{
	if (a == Type::Unknown || b == Type::Unknown)
		return Type::Unknown;

	if (a == Type::Float || b == Type::Float)
		return Type::Float;

	unsigned mrank = std::max(rank(a), rank(b));

	if (mrank < 4) {
		return (isSigned(a) && isSigned(b))
				|| (isSigned(a) && !isSigned(b) && rank(a) > rank(b))
				|| (!isSigned(a) && isSigned(b) && rank(a) < rank(b))
			? Type::Int32
			: Type::UInt32;
	} else {
		return (isSigned(a) && isSigned(b))
				|| (isSigned(a) && !isSigned(b) && rank(a) > rank(b))
				|| (!isSigned(a) && isSigned(b) && rank(a) < rank(b))
			? Type::Int64
			: Type::UInt64;
	}
}

bool isAssignableFrom(Type to, Type from)
{
	switch (to) {
	case Type::Bool:
	case Type::Float:
		return from == to;

	case Type::UInt8:
	case Type::UInt16:
	case Type::UInt32:
	case Type::UInt64:
		return isIntType(from);

	case Type::Int8:
	case Type::Int16:
	case Type::Int32:
	case Type::Int64:
		return isIntType(from) && rank(to) >= rank(from);

	default:
		throw "invalid type";
	}
}

bool isSigned(Type t)
{
	switch (t) {
	case Type::Int8:
	case Type::Int16:
	case Type::Int32:
	case Type::Int64:
	case Type::Float:
		return true;

	default:
		return false;
	}
}

bool isIntType(Type t)
{
	switch (t) {
	case Type::UInt8:
	case Type::UInt16:
	case Type::UInt32:
	case Type::UInt64:
	case Type::Int8:
	case Type::Int16:
	case Type::Int32:
	case Type::Int64:
		return true;

	default:
		return false;
	}
}

unsigned widthOf(Type t)
{
	switch (t) {
	case Type::Bool:
		return 1;

	case Type::UInt8:
	case Type::Int8:
		return 8;

	case Type::UInt16:
	case Type::Int16:
		return 16;

	case Type::UInt32:
	case Type::Int32:
		return 32;

	case Type::UInt64:
	case Type::Int64:
		return 64;

	case Type::Float:
		return 0;

	default: throw "unknown type";
	}
}

}
}
