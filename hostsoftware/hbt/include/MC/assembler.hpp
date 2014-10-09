#ifndef INCLUDE_MC_ASSEMBLER_HPP_B133472E3A0E2CF8
#define INCLUDE_MC_ASSEMBLER_HPP_B133472E3A0E2CF8

#include <cstdint>
#include <vector>

#include "Util/memorybuffer.hpp"

namespace hbt {

namespace mc {

class Program;

hbt::util::MemoryBuffer assemble(const Program& program);

}
}

#endif
