# -*- mode: cmake; -*-
project(hexanode-backend)
cmake_minimum_required(VERSION "2.8" FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../../cmake_modules ${CMAKE_MODULE_PATH})
include(HexabusCommon)


if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND CMAKE_FIND_ROOT_PATH ${HXB_HOME})
  list(APPEND CMAKE_FIND_ROOT_PATH ${LIBKLIO_HOME})
endif()

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

# find libklio
find_package(Libklio)
if( NOT LIBKLIO_FOUND)
  message(FATAL_ERROR "libklio is required.You may set LIBKLIO_HOME")
endif( NOT LIBKLIO_FOUND)

FIND_PACKAGE(OpenSSL REQUIRED)
FIND_PACKAGE(CppNetlib REQUIRED)
FIND_PACKAGE(LibHexabus REQUIRED)
FIND_PACKAGE(Sqlite REQUIRED)
include(FindJson)
include(FindCurl)
FIND_PACKAGE(Gnutls REQUIRED)
find_package(Alsa)

if (NOT HXB_FOUND)
  message(FATAL_ERROR "libhexabus ist required.")
endif()

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

