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
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND CMAKE_FIND_ROOT_PATH ${LIBKLIO_HOME})
endif()

include ( ProjectInfo )
include ( CompilerFlags )
include (UseCodeCoverage)

set(CMAKE_CXX_FLAGS "${CXXFLAGS} -std=c++11")

#set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-function")
#set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused -Woverloaded-virtual")

#set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wunused-function -Wunused")
#set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Woverloaded-virtual ")

# Check for some compiler stuff
#include(CMakeBackwardCompatibilityCXX)

# We depend on boost >= 1.46.1 because of the updated filesystem
# library.
SET(Boost_ADDITIONAL_VERSIONS "1.46.1" "1.47" "1.47.1" "1.48.0" "1.48"
  "1.49.0" "1.49" "1.50.0" "1.50")
set(Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.46.1 REQUIRED COMPONENTS test_exec_monitor program_options filesystem system date_time thread)

# find libklio
find_package(Libklio)
if( LIBKLIO_FOUND )
  FIND_PACKAGE(Sqlite)
  FIND_PACKAGE(OpenSSL REQUIRED)
  FIND_PACKAGE(Gnutls REQUIRED)
  add_definitions ("-DBOOST_FILESYSTEM_VERSION=3")

  # libjson
  include(FindJsonCpp)
  if( NOT JSONCPP_FOUND)
    message(FATAL_ERROR "libjson ist required. Install json or call cmake -DJSON_HOME=path_to_json_install")
  endif( NOT JSONCPP_FOUND)
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
endif( LIBKLIO_FOUND )

if( NOT WITHOUT_CPACK )
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
  SET(CPACK_PACKAGE_EXECUTABLES "hexaswitch;Hexabus node interrogation utility" "hexalog;Hexabus logging utility" "hexaupload;Hexabus statemachine upload utility" "hexapair;Hexabus device pairing utility" "hexatimed;Hexabus datetime broadcasting daemon")
  SET(CPACK_STRIP_FILES ON)

  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.6.0), libc6 (>= 2.13)")
  if(LIBKLIO_FOUND)
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libsqlite3-0 (>= 3.7), libgnutls26 (>= 2.12)")
  endif()
  set(CPACK_SECTION "net")
  include(CPackTools)
endif()


set(HXB_SHARED "${CMAKE_SOURCE_DIR}/../../shared")
message(STATUS "HXB_SHARED:  ${HXB_SHARED}")
configure_file("${HXB_SHARED}/hexabus_types.h" "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/hexabus_types.h" @COPY_ONLY)
configure_file("${HXB_SHARED}/hexabus_statemachine_structs.h" "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/hexabus_statemachine_structs.h" @COPY_ONLY)
configure_file("${HXB_SHARED}/sm_types.h" "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/sm/sm_types.h" @COPY_ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/hexabus_types.h" DESTINATION include/libhexabus)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/hexabus_statemachine_structs.h" DESTINATION include/libhexabus)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/sm/sm_types.h" DESTINATION include/libhexabus/sm)

# Reflect the package structure
add_subdirectory(libhexabus)
add_subdirectory(src)
add_subdirectory(tests)

# enable unit testing
include(CTest)
enable_testing()

# add some files to the installation target
INSTALL(FILES README.md LICENSE.txt DESTINATION share/doc/hexaswitch)
INSTALL(FILES share/endpoint_registry DESTINATION share/libhexabus)

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
  message(" libklio include: ${LIBKLIO_INCLUDE_DIRS}, lib: ${LIBKLIO_LIBRARY}")
  message(" curl    include: ${CURL_INCLUDE_DIR}, lib: ${CURL_LIBRARIES}")
  message(" gnutls  include: ${GNUTLS_INCLUDE_DIR}, lib: ${GNUTLS_LIBRARIES}")
  message(" rocksdb include: ${ROCKSDB_INCLUDE_DIR}, lib: ${ROCKSDB_LIBRARY}")
endif( LIBKLIO_FOUND )
message("  extended logging: ${ENABLE_LOGGING}")

if( NOT LIBKLIO_FOUND )
  message(WARNING "libklio not found. hexalog will be disabled in this build.")
endif( NOT LIBKLIO_FOUND )
message("  rocksdb: ${LIBKLIO_ENABLE_ROCKSDB}")
message("  mysmartgrid: ${LIBKLIO_ENABLE_MSG}")
