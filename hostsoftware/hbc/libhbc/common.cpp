#include "common.hpp"
#include <sstream>
#include <iostream>
#include <stdlib.h>

using namespace hexabus;

const std::string VersionInfo::getVersion() {
  std::ostringstream oss;
  oss << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
  return oss.str();
}

// singleton housekeeping
GlobalOptions* GlobalOptions::instance = NULL;

void GlobalOptions::deleteInstance() {
  std::cout << "d";
  delete instance;
  instance = NULL;
}

GlobalOptions::GlobalOptions() {
  std::cout << "n";
  atexit(&deleteInstance);
}

GlobalOptions* GlobalOptions::getInstance() {
  if(instance == NULL)
    instance = new GlobalOptions();
  return instance;
}
// ========= ============

bool GlobalOptions::getVerbose() {
  return verbose;
}

void GlobalOptions::setVerbose(bool v) {
  verbose = v;
}
