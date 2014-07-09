#ifndef INCLUDE_IR_PROGRAM__PRINTER_HPP_EFB6A48BBC3894A9
#define INCLUDE_IR_PROGRAM__PRINTER_HPP_EFB6A48BBC3894A9

#include <string>

namespace hbt {
namespace ir {

class Program;

std::string prettyPrint(const Program& program);

}
}

#endif
