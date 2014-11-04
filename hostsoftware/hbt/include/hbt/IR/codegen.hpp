#ifndef CODEGEN__HPP_EAE16CDABB4FC479
#define CODEGEN__HPP_EAE16CDABB4FC479

#include <memory>

namespace hbt {

namespace mc {
class Builder;
}

namespace ir {

class Module;

std::unique_ptr<mc::Builder> generateMachineCode(const Module& module);

}
}

#endif
