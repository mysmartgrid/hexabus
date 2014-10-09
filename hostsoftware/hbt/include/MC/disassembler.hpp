#ifndef INCLUDE_MC_DISASSEMBLER_HPP_B133472E3A0E2CF8
#define INCLUDE_MC_DISASSEMBLER_HPP_B133472E3A0E2CF8

#include <cstdint>
#include <memory>
#include <vector>

namespace hbt {

namespace util {
class MemoryBuffer;
}

namespace mc {

class Program;

std::unique_ptr<Program> disassemble(const hbt::util::MemoryBuffer& program, bool ignoreInvalid = false);

}
}

#endif
