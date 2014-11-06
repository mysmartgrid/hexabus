# -*- mode: cmake; -*-
project(hexinfo)
cmake_minimum_required(VERSION "2.8" FATAL_ERROR)
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

if( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
   message(STATUS "Do not run cmake in the source directory")
   message(STATUS "create an extra binary directory")
   message(FATAL_ERROR "Exiting cmake here.")
endif( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

# have the full monty in makefiles
set(CMAKE_VERBOSE_MAKEFILE true)

SET(ENABLE_LOGGING 1)

set(V_MAJOR 0)
set(V_MINOR 2)
set(V_PATCH 0)

# add a path where some libraries might be stored
set(CMAKE_ADDITIONAL_PATH "$ENV{CMAKE_ADDITIONAL_PATH}" CACHE PATH "Path where many locally installed libraries can be found")

###################################################################################################
# No user settings below
###################################################################################################

# Where are the additional libraries installed? Note: provide includes
# path here, subsequent checks will resolve everything else
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${CMAKE_ADDITIONAL_PATH}/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_ADDITIONAL_PATH}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND CMAKE_FIND_ROOT_PATH ${HXB_HOME})
  list(APPEND CMAKE_FIND_ROOT_PATH ${HBC_HOME})
endif()

include ( CompilerFlags )
include (UseCodeCoverage)

set(CMAKE_CXX_FLAGS "${CXXFLAGS} -std=c++11")

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

FIND_PACKAGE(CLN)
FIND_PACKAGE(HBT)
FIND_PACKAGE(LibHexabus)
set(Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.54 REQUIRED COMPONENTS test_exec_monitor program_options filesystem system thread)

# Reflect the package structure
add_subdirectory(src)

# enable unit testing
include(CTest)
enable_testing()

# add some files to the installation target
INSTALL(FILES README.md LICENSE.txt DESTINATION share/doc/hexinfo)

# CPACK packaging
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "hexinfo")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.8.0), libc6 (>= 2.13), libcln6 (>= 1.3.3)")

set(CPACK_PACKAGE_EXECUTABLES "hexinfo" "Hexabus device discovery and info tool")
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_STRIP_FILES "bin/hexinfo")
set(CPACK_SECTION "net")
include(CPackTools)

### print some output for the user
message("")
message("***** Build configuration *****")
message("  install prefix: ${CMAKE_INSTALL_PREFIX}")
message("  hexabus include: ${HXB_INCLUDE_DIR}")
message("  hexabus library: ${HXB_LIBRARIES}")
message("  hbt include: ${HBT_INCLUDE_DIR}")
message("  hbt libraries: ${HBT_LIBRARIES}")
message("  boost include: ${Boost_INCLUDE_DIRS}")
message("  boost libraries: ${Boost_LIBRARIES}")
message("  extended logging: ${ENABLE_LOGGING}")

