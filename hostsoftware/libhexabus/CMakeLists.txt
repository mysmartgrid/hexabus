# -*- mode: cmake; -*-
project(libhexabus)
cmake_minimum_required(VERSION "2.8" FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})
include(HexabusCommon)

###################################################################################################
# No user settings below
###################################################################################################

if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND CMAKE_FIND_ROOT_PATH ${LIBKLIO_HOME})
  list(APPEND CMAKE_FIND_ROOT_PATH ${LIBMYSMARTGRID_HOME})
endif()

# find libklio
find_package(Libklio)
if( LIBKLIO_FOUND )
  FIND_PACKAGE(LibMysmartGrid REQUIRED)
  add_definitions ("-DBOOST_FILESYSTEM_VERSION=3")
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
configure_file("${HXB_SHARED}/sm_types.h" "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/sm/sm_types.h" @COPY_ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libhexabus/hexabus_types.h" DESTINATION include/libhexabus)
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
#  message(" sqlite3        include: ${SQLITE3_INCLUDE_DIR}, lib: ${SQLITE3_LIBRARIES}")
  message(" libklio        include: ${LIBKLIO_INCLUDE_DIRS}, lib: ${LIBKLIO_LIBRARY}")
  message(" libmysmartgrid include: ${LIBMYSMARTGRID_INCLUDE_DIRS}, lib: ${LIBMYSMARTGRID_LIBRARY}")
#  message(" curl    include: ${CURL_INCLUDE_DIR}, lib: ${CURL_LIBRARIES}")
#  message(" gnutls  include: ${GNUTLS_INCLUDE_DIR}, lib: ${GNUTLS_LIBRARIES}")
#  message(" rocksdb include: ${ROCKSDB_INCLUDE_DIR}, lib: ${ROCKSDB_LIBRARY}")
endif( LIBKLIO_FOUND )
message("  extended logging: ${ENABLE_LOGGING}")

if( NOT LIBKLIO_FOUND )
  message(WARNING "libklio not found. hexalog will be disabled in this build.")
endif( NOT LIBKLIO_FOUND )
message("  rocksdb: ${LIBKLIO_ENABLE_ROCKSDB}")
message("  mysmartgrid: ${LIBKLIO_ENABLE_MSG}")
