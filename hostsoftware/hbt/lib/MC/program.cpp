#include "MC/program.hpp"

namespace hbt {
namespace mc {

Program::~Program()
{
	for (Instruction* i : _instructions)
		delete i;
}

}
}
