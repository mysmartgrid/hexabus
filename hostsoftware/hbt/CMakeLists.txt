# -*- mode: cmake; -*-
project(hbt)
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

# use ctest
ENABLE_TESTING()

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

include ( ProjectInfo )
include ( CompilerFlags )
include (UseCodeCoverage)

set(CMAKE_CXX_FLAGS "${CXXFLAGS} -std=c++11")

if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	# g++ fails to produce sensible warnings about missng field initializers in C++11 mode
	# because it warns *way* too much about code that is in no way incorrect
	# see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=36750#c11
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
endif()

set(Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.54 REQUIRED COMPONENTS test_exec_monitor program_options filesystem regex system)

find_package(CLN)

# Reflect the package structure
include_directories(${CMAKE_SOURCE_DIR}/include)
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)

# enable unit testing
include(CTest)
enable_testing()

set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hexabus compiler toolkit")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.8.0), libc6 (>= 2.13), libcln6 (>= 1.3.3)")
set(CPACK_PACKAGE_EXECUTABLES "hbc" "Hexabus statemachine compiler")
set(CPACK_PACKAGE_EXECUTABLES "hbt-as" "Hexabus assembler")
set(CPACK_PACKAGE_EXECUTABLES "hbt-dis" "Hexabus disassembler")
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_STRIP_FILES "bin/hbc" "bin/hbt-as" "bin/hbt-dis")
include(CPackTools)

### print some output for the user
message("")
message("***** Configuration *****")
message("  prefix: ${CMAKE_INSTALL_PREFIX}")
message("  boost include: ${Boost_INCLUDE_DIRS}")
message("  boost libraries shortcut: ${Boost_LIBRARIES}")
message("  boost program options lib: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
message("  boost system lib: ${Boost_SYSTEM_LIBRARY}")
message("  boost filesystem lib: ${Boost_FILESYSTEM_LIBRARY}")
message("  extended logging: ${ENABLE_LOGGING}")
