project(hxbnm)
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

if(CMAKE_TOOLCHAIN_FILE)
endif()

include (CompilerFlags)
include (UseCodeCoverage)

find_package(LibNL REQUIRED)
find_package(LibGENL REQUIRED)
find_package(LibNLRoute REQUIRED)

# We depend on boost >= 1.46.1 because of the updated filesystem
# library.
SET(Boost_ADDITIONAL_VERSIONS "1.46.1" "1.47" "1.47.1" "1.48.0" "1.48"
  "1.49.0" "1.49" "1.50.0" "1.50")
set(Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.46.1 REQUIRED)



add_subdirectory(eep)
add_subdirectory(rflib)
add_subdirectory(src)

# enable unit testing
include(CTest)
enable_testing()

if( NOT WITHOUT_CPACK )
  # CPACK packaging
  include(InstallRequiredSystemLibraries)
  set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
  set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
  set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hexabus networt manager")
  #  set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
  #  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
  set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})
  SET(CPACK_PACKAGE_EXECUTABLES "hxbnm;Hexabus network management utility")
  SET(CPACK_STRIP_FILES ON)

  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.6.0), libc6 (>= 2.13), libnl-3-200, libnl-genl-3-200, libnl-route-3-200")
  set(CPACK_SECTION "net")
  include(CPackTools)
endif()

### print some output for the user
message("")
message("***** Configuration *****")
message("  prefix: ${CMAKE_INSTALL_PREFIX}")
message("  boost include: ${Boost_INCLUDE_DIRS}")
message("  libnl3 include: ${LIBNL_INCLUDE_DIR}, lib: ${LIBNL_LIBRARIES}")
message("  libgenl3 include: ${LIBGENL_INCLUDE_DIR}, lib: ${LIBGENL_LIBRARIES}")
message("  libnlroute3 include: ${LIBNLROUTE_INCLUDE_DIR}, lib: ${LIBNLROUTE_LIBRARIES}")
message("  extended logging: ${ENABLE_LOGGING}")
