#include "hbt/Util/fatal.hpp"

#include <stdexcept>

namespace {
class UnreachableCodeReached : std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};
}

void hbt::hbt_unreachable(const char* msg)
{
	throw UnreachableCodeReached(msg);
}
