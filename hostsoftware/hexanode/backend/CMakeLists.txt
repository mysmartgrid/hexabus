# -*- mode: cmake; -*-
project(hexanode-backend)
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
#ENABLE_TESTING()

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
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../../cmake_modules ${CMAKE_MODULE_PATH})

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND CMAKE_FIND_ROOT_PATH ${HXB_HOME})
  list(APPEND CMAKE_FIND_ROOT_PATH ${LIBKLIO_HOME})
endif()

include ( CompilerFlags )
include (UseCodeCoverage)

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

# find libklio
find_package(Libklio)
if( NOT LIBKLIO_FOUND)
  message(FATAL_ERROR "libklio is required.You may set LIBKLIO_HOME")
else()
  # RocksDB
  # check if the compiler version supports c++ 2011
  #if( ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.7")  
  #  include(FindRocksDB)
  #  if(ROCKSDB_FOUND)
  #    include_directories(${ROCKSDB_INCLUDE_DIR})
  #  else(ROCKSDB_FOUND)
  #    set(ROCKSDB_INCLUDE_DIR "")
  #    message(STATUS "RocksDB support is disabled because the library is not found. Install RocksDB or call cmake -DROCKSDB_HOME=path_to_rocksdb_install in order to enable it.")
  #  endif(ROCKSDB_FOUND)
  #qendif()
endif( NOT LIBKLIO_FOUND)

FIND_PACKAGE(OpenSSL REQUIRED)
FIND_PACKAGE(CppNetlib REQUIRED)
FIND_PACKAGE(LibHexabus REQUIRED)
#FIND_PACKAGE(Libklio REQUIRED)
FIND_PACKAGE(Sqlite REQUIRED)
include(FindJson)
include(FindCurl)
FIND_PACKAGE(Gnutls REQUIRED)
find_package(Alsa)

if (NOT HXB_FOUND)
  message(FATAL_ERROR "libhexabus ist required.")
endif()
#FIND_PACKAGE(RtMidi)
# We depend on boost >= 1.46.1 because of the updated filesystem
# library.
SET(Boost_ADDITIONAL_VERSIONS "1.46.1" "1.47" "1.47.1" "1.48.0" "1.48"
  "1.49.0" "1.49" "1.50.0" "1.50")
set(Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.46.1 REQUIRED COMPONENTS 
  test_exec_monitor program_options filesystem system thread)

# Reflect the package structure
add_subdirectory(libhexanode)
add_subdirectory(RtMidi)
add_subdirectory(src)

# add some files to the installation target
INSTALL(FILES 
  README.md LICENSE.txt 
  DESTINATION share/doc/hexanode)

INSTALL(FILES
  etc/simulation.ini
  DESTINATION share/doc/hexanode/simulation.ini)

INSTALL(PROGRAMS etc/init.d/reverse-ssh DESTINATION /etc/init.d)
INSTALL(FILES etc/reverse-ssh DESTINATION /etc)
INSTALL(FILES etc/cron.d/hexabus_msg_heartbeat DESTINATION /etc/cron.d)
INSTALL(FILES msg_CA.crt DESTINATION /usr/local/share/ca-certificates)
INSTALL(PROGRAMS src/hexabus_msg_heartbeat DESTINATION bin)
INSTALL(PROGRAMS etc/service/hexabus_msg_bridge/run DESTINATION /etc/service/hexabus_msg_bridge)
INSTALL(PROGRAMS etc/service/hexabus_msg_bridge/log/run DESTINATION /etc/service/hexabus_msg_bridge/log)
INSTALL(PROGRAMS etc/service/hexanode-backend/run DESTINATION /etc/service/hexanode-backend)
INSTALL(PROGRAMS etc/service/hexanode-backend/log/run DESTINATION /etc/service/hexanode-backend/log)


# CPACK packaging
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Backend utilities for a virtual hexabus device")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.6.0), libc6 (>= 2.13), libsqlite3-0 (>= 3.7), libgnutls26 (>= 2.12), libssl1.0.0, libasound2, python3 (>= 3.2), libhexabus (>= 0.6.0.200), ca-certificates")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/conffiles;${CMAKE_CURRENT_SOURCE_DIR}/debian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")

set(CPACK_PACKAGE_EXECUTABLES "hexanode" "A virtual hexabus device.")
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_STRIP_FILES "bin/hexanode")
set(CPACK_SECTION "net")
include(CPackTools)

### print some output for the user
message("")
message("***** Build configuration *****")
message("  install prefix: ${CMAKE_INSTALL_PREFIX}")
message("  hexabus include: ${HXB_INCLUDE_DIR}")
message("  hexabus library: ${HXB_LIBRARIES}")
message("  boost include: ${Boost_INCLUDE_DIRS}")
message("  boost libraries: ${Boost_LIBRARIES}")
message("  libklio include: ${LIBKLIO_INCLUDE_DIRS}, lib: ${LIBKLIO_LIBRARY}")
message("  sqlite3 include: ${SQLITE3_INCLUDE_DIR}, lib: ${SQLITE3_LIBRARIES}")
message("  rocksdb include: ${ROCKSDB_INCLUDE_DIR}, lib: ${ROCKSDB_LIBRARY}")
message("  extended logging: ${ENABLE_LOGGING}")

