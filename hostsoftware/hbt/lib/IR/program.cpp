#include "IR/program.hpp"

namespace hbt {
namespace ir {

Program::~Program()
{
	for (Instruction* i : _instructions)
		delete i;
}

}
}
