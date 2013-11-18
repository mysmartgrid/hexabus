# -*- mode: cmake; -*-
project(hbc)
cmake_minimum_required(VERSION "2.8" FATAL_ERROR)
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

# have the full monty in makefiles
set(CMAKE_VERBOSE_MAKEFILE true)

SET(ENABLE_LOGGING 1)

set(V_MAJOR 0)
set(V_MINOR 1)
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

include ( CompilerFlags )
include (UseCodeCoverage)

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

# We depend on boost >= 1.46.1 because of the updated filesystem
# library.
set(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost 1.46.1 COMPONENTS program_options filesystem system)

# Reflect the package structure
add_subdirectory(libhbc)
add_subdirectory(src)

# add some files to the installation target
INSTALL(FILES README.txt LICENSE.txt DESTINATION share/doc/hbc)

# CPACK packaging
INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HexaBus Compiler")
SET(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.6.0), libc6 (>= 2.13)")

SET(CPACK_PACKAGE_EXECUTABLES "hbc" "Compiler for HexaBus State Machines.")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_STRIP_FILES "bin/hbc")
include(CPackTools)

### print some output for the user
message("")
message("***** Configuration *****")
message("  prefix: ${CMAKE_INSTALL_PREFIX}")
message("  boost include: ${Boost_INCLUDE_DIRS}")
message("  boost program options lib: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
message("  boost system lib: ${Boost_SYSTEM_LIBRARY}")
message("  extended logging: ${ENABLE_LOGGING}")

