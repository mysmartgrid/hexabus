# -*- mode: cmake; -*-
project(hexabridge)
cmake_minimum_required(VERSION "2.8" FATAL_ERROR)
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

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
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wno-non-virtual-dtor -g -fomit-frame-pointer")

set(CMAKE_CXX_FLAGS_EXPERIMENTAL "-O0 -g -ggdb -Wall -Werror -W -Wshadow")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-variable")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-parameter")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-function")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused -Woverloaded-virtual")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wno-system-headers")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wno-non-virtual-dtor")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -pg -fprofile-generate ")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -fprofile-arcs -ftest-coverage")
# gprof and gcov support
set(CMAKE_CXX_FLAGS_PROFILE "-O0 -g -ggdb -W -Wreturn-type -Wno-shadow")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wunused-variable -Wunused-parameter")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wunused-function -Wunused")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Woverloaded-virtual -Wno-system-headers")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wno-non-virtual-dtor -pg -fprofile-arcs")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -ftest-coverage")

# Releases are made with the release build. Optimize code.
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-non-virtual-dtor -Werror")

# allways set these flags
set(CMAKE_CXX_FLAGS "-Wall ")

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

# Find Boost.
FIND_PACKAGE(Boost 1.40.0 REQUIRED COMPONENTS system)

# Reflect the package structure
add_subdirectory(src)

# add some files to the installation target
#INSTALL(FILES README.txt LICENSE.txt DESTINATION share/doc/hexabridge)

# CPACK packaging
INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "hexabridge")
SET(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "xxx@itwm.fhg.de")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "xxx@itwm.fhg.de")
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
SET(CPACK_PACKAGE_EXECUTABLES "hexabridge" "Simple bridging Daemon between hexabus Networks.")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_STRIP_FILES "bin/hexabridge")

set(CPACK_SET_DESTDIR On)
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_GENERATOR "DEB;TBZ2")
set(CPACK_SECTION "net")

INCLUDE(CPack)

### print some output for the user
message("")
message("***** Configuration *****")
message("  prefix: ${CMAKE_INSTALL_PREFIX}")
message("  Boost lib: ${Boost_INCLUDE_DIRS}")
