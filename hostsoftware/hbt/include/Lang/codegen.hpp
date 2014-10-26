#ifndef INCLUDE_LANG_CODEGEN_HPP_127703CDCA9B55E7
#define INCLUDE_LANG_CODEGEN_HPP_127703CDCA9B55E7

#include <map>
#include <memory>
#include <set>

namespace hbt {

namespace mc {
class Program;
}

namespace lang {

class Device;
class MachineDefinition;
class TranslationUnit;

std::map<const Device*, std::set<MachineDefinition*>> collectMachines(TranslationUnit& tu);

std::unique_ptr<mc::Program> generateMachineCodeFor(TranslationUnit& tu);

}
}

#endif
