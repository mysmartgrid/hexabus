# -*- mode: cmake; -*-
project(hbt)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})
include(HexabusCommon)

###################################################################################################
# No user settings below
###################################################################################################

# Reflect the package structure
include_directories(${CMAKE_SOURCE_DIR}/include)
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)

set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hexabus compiler toolkit")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.8.0), libc6 (>= 2.13)")
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
