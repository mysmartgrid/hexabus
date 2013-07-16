# -*- mode: cmake; -*-
project(libhexabus)
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
set(V_MINOR 6)
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
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_MODULE_PATH})

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND CMAKE_FIND_ROOT_PATH ${LIBKLIO_HOME})
endif()

include (UseCodeCoverage)

# Detect the system we're compiling on
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(HAS_MACOS 1)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(HAS_MACOS 0)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(HAS_LINUX 1)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(HAS_LINUX 0)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


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
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-non-virtual-dtor")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unused-but-set-variable")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unused-variable")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-delete-non-virtual-dtor")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")

# allways set these flags
set(CMAKE_CXX_FLAGS "-Wall ")
# disable ipa-sra, as this breaks binaries on raspberry
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-ipa-sra")
endif()

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

# find libklio
find_package(Libklio)
if( LIBKLIO_FOUND )
  FIND_PACKAGE(Sqlite)
  FIND_PACKAGE(OpenSSL REQUIRED)
  FIND_PACKAGE(Gnutls REQUIRED)
  add_definitions ("-DBOOST_FILESYSTEM_VERSION=3")
endif( LIBKLIO_FOUND )

# We depend on boost >= 1.46.1 because of the updated filesystem
# library.
SET(Boost_ADDITIONAL_VERSIONS "1.46.1" "1.47" "1.47.1" "1.48.0" "1.48"
  "1.49.0" "1.49" "1.50.0" "1.50")
set(Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.46.1 REQUIRED COMPONENTS test_exec_monitor program_options filesystem system date_time thread)

# libjson
include(FindJson)
if( NOT JSON_FOUND)
  message(FATAL_ERROR "libjson ist required. Install json or call cmake -DJSON_HOME=path_to_json_install")
endif( NOT JSON_FOUND)
include_directories(${JSON_INCLUDE_DIR})

# check for curl
IF(WIN32)
  add_definitions(-DCURL_STATICLIB)
  include(FindCURL_WIN32)
else(WIN32)
  # add_definitions(-DCURL_STATICLIB)
  include(FindCurl)
  include(FindGnutls)
endif(WIN32)

# Reflect the package structure
add_subdirectory(libhexabus)
add_subdirectory(src)
add_subdirectory(etc)
add_subdirectory(tests)

# enable unit testing
include(CTest)
enable_testing()

# add some files to the installation target
INSTALL(FILES README.md LICENSE.txt DESTINATION share/doc/hexaswitch)

# CPACK packaging
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libhexabus")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})
SET(CPACK_PACKAGE_EXECUTABLES "hexaswitch;Hexabus node interrogation utility" "hexalog;Hexabus logging utility" "hexaupload;Hexabus statemachine upload utility")
SET(CPACK_STRIP_FILES ON)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.6.0), libc6 (>= 2.13)")

set(CPACK_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/conffiles;${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")
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
if( LIBKLIO_FOUND )
  message(" sqlite3 include: ${SQLITE3_INCLUDE_DIR}, lib: ${SQLITE3_LIBRARIES}")
  message(" libklio include: ${LIBKLIO_INCLUDE_DIR}, lib: ${LIBKLIO_LIBRARY}")
  message(" curl    include: ${CURL_INCLUDE_DIR}, lib: ${CURL_LIBRARIES}")
  message(" gnutls  include: ${GNUTLS_INCLUDE_DIR}, lib: ${GNUTLS_LIBRARIES}")
endif( LIBKLIO_FOUND )
message("  extended logging: ${ENABLE_LOGGING}")

if( NOT LIBKLIO_FOUND )
  message(WARNING "libklio not found. hexalog will be disabled in this build.")
endif( NOT LIBKLIO_FOUND )
