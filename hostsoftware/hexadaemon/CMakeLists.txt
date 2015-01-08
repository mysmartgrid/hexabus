# -*- mode: cmake; -*-
project(hexinfo)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})
include(HexabusCommon)

###################################################################################################
# No user settings below
###################################################################################################

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

find_package(LibHexabus)
find_package(UCI)

configure_file(${CMAKE_SOURCE_DIR}/configure.h.in ${CMAKE_BINARY_DIR}/configure.h)

# Reflect the package structure
add_subdirectory(src)

# add some files to the installation target
INSTALL(FILES README.md LICENSE.txt DESTINATION share/doc/hexadaemon)

# CPACK packaging
INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mathias Dalheimer")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "hexinfo")
SET(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "support@mysmartgrid.de")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "support@mysmartgrid.de")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
SET(CPACK_PACKAGE_EXECUTABLES "hexinfo" "Hexabus device discovery and info tool")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_STRIP_FILES "bin/hexinfo")

set(CPACK_SET_DESTDIR On)
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_GENERATOR "DEB;TBZ2")
set(CPACK_SECTION "net")

INCLUDE(CPack)

### print some output for the user
message("")
message("***** Build configuration *****")
message("  install prefix: ${CMAKE_INSTALL_PREFIX}")
message("  hexabus include: ${HXB_INCLUDE_DIR}")
message("  hexabus library: ${HXB_LIBRARIES}")
message("  uci include: ${UCI_INCLUDE_DIR}")
message("  uci library: ${UCI_LIBRARY}")
message("  boost include: ${Boost_INCLUDE_DIRS}")
message("  boost libraries: ${Boost_LIBRARIES}")
message("  extended logging: ${ENABLE_LOGGING}")

