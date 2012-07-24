#include "common.hpp"
#include <sstream>

using namespace hexabus;

const std::string VersionInfo::getVersion() {
  std::ostringstream oss;
  oss << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
  return oss.str();
}

