#ifndef INCLUDE_MC_ASSEMBLER_HPP_B133472E3A0E2CF8
#define INCLUDE_MC_ASSEMBLER_HPP_B133472E3A0E2CF8

#include <cstdint>
#include <vector>

namespace hbt {

namespace ir {
class Program;
}

namespace mc {

std::vector<uint8_t> assemble(const ir::Program& program);

}
}

#endif
