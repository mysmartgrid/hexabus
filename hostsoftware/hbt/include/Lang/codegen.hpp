#ifndef INCLUDE_LANG_CODEGEN_HPP_127703CDCA9B55E7
#define INCLUDE_LANG_CODEGEN_HPP_127703CDCA9B55E7

#include <memory>

namespace hbt {

namespace mc {
class Program;
}

namespace lang {

class TranslationUnit;

std::unique_ptr<mc::Program> generateMachineCodeFor(TranslationUnit& tu);

}
}

#endif
