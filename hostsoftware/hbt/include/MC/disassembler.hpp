#ifndef INCLUDE_MC_DISASSEMBLER_HPP_B133472E3A0E2CF8
#define INCLUDE_MC_DISASSEMBLER_HPP_B133472E3A0E2CF8

#include <cstdint>
#include <memory>
#include <vector>

namespace hbt {

namespace ir {
class Program;
}

namespace util {
class MemoryBuffer;
}

namespace mc {

std::unique_ptr<ir::Program> disassemble(const hbt::util::MemoryBuffer& program, bool ignoreInvalid = false);

}
}

#endif
