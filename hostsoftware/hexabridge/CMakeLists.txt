# -*- mode: cmake; -*-
project(hexabridge)
string(TOUPPER ${PROJECT_NAME} UPPER_PROJECT_NAME)
cmake_minimum_required(VERSION "2.6" FATAL_ERROR)
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

# have the full monty in makefiles
set(CMAKE_VERBOSE_MAKEFILE true)

set(V_MAJOR 0)
set(V_MINOR 0)
set(V_PATCH 1)

# use ctest
include(CTest)
enable_testing()

# add a path where some libraries might be stored
set(CMAKE_ADDITIONAL_PATH "$ENV{CMAKE_ADDITIONAL_PATH}" CACHE PATH "Path where many locally installed libraries can be found")

###################################################################################################
# No user settings below
###################################################################################################

add_definitions("-std=c++0x")

# Where are the additional libraries installed? Note: provide includes
# path here, subsequent checks will resolve everything else
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${CMAKE_ADDITIONAL_PATH}/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_ADDITIONAL_PATH}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wno-non-virtual-dtor -g -fomit-frame-pointer")

set(CMAKE_CXX_FLAGS_EXPERIMENTAL "-O0 -g -ggdb -Wall  -W -Wshadow")
#set(CMAKE_CXX_FLAGS_EXPERIMENTAL "-O0 -g -ggdb -Wall -Werror -W -Wshadow")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-variable")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-parameter")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-function")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused -Woverloaded-virtual")
#set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wno-system-headers")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wno-non-virtual-dtor")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -pg -fprofile-generate ")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -fprofile-arcs -ftest-coverage")

# gprof and gcov support
set(CMAKE_CXX_FLAGS_PROFILE "-O0 -g -ggdb -W -Wreturn-type -Wno-shadow")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wunused-variable -Wunused-parameter")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wunused-function -Wunused")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Woverloaded-virtual -Wno-system-headers")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wno-non-virtual-dtor")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RROFILE} -pg -fprofile-arcs")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -ftest-coverage")

# Releases are made with the release build. Optimize code and strip binaries.
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-non-virtual-dtor")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unused-but-set-variable")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unused-variable")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unused-result")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-sign-compare")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-uninitialized")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-attributes")

if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations")
else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
endif()

# always set these flags
set(CMAKE_CXX_FLAGS "-Wall")

# We depend on boost >= 1.46.1 because of the updated filesystem
# library. The boost version detection is a mess, see 
# $ cmake --help-module FindBoost
# , section " The mess that is Boost_ADDITIONAL_VERSIONS (sorry?)"
SET(Boost_ADDITIONAL_VERSIONS "1.46.1" "1.47" "1.47.1" "1.48.0" "1.48"
  "1.49.0" "1.49" "1.50.0" "1.50")
set (Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.46.1 
  COMPONENTS test_exec_monitor program_options filesystem system date_time)

find_package(LibDiscovergy)

find_package(LibHexabus)

include_directories(${CMAKE_BINARY_DIR})

# Check for some compiler stuff
include(CMakeBackwardCompatibilityCXX)

# Reflect the package structure
add_subdirectory(src)

#CPack Packaging
INSTALL(FILES 
  LICENSE_1_0.txt LICENSE 
  DESTINATION share/doc/hexabridge)


INSTALL(PROGRAMS etc/service/hexabridge/run DESTINATION /etc/service/hexabridge)
INSTALL(PROGRAMS etc/service/hexabridge/log/run DESTINATION /etc/service/hexabridge/log)

INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/src/hexabridge DESTINATION bin)

include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bridge that pushes values from msg to hexabus.")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 4.6.0), libc6 (>= 2.13), libsqlite3-0 (>= 3.7), libgnutls26 (>= 2.12), libssl1.0.0, libcurl3, libhexabus (>= 0.6.0.200), ca-certificates")
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/conffiles;${CMAKE_CURRENT_SOURCE_DIR}/debian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/conffiles;${CMAKE_CURRENT_SOURCE_DIR}/d    ebian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")


set(CPACK_PACKAGE_EXECUTABLES "hexabridge" "Bridge that pushes values from msg to hexabus.")
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_STRIP_FILES "bin/hexabridge")
set(CPACK_SECTION "net")
include(CPackTools)



### print some output for the user
message("")
message("***** Configuration *****")
message("  prefix: ${CMAKE_INSTALL_PREFIX}")
message("  boost include: ${Boost_INCLUDE_DIRS}")
message("  boost libraries shortcut: ${Boost_LIBRARIES}")
message("  json: -L${JSON_LIBRARY} -I${JSON_INCLUDE_DIR}")
message("  sqlite3 include: ${SQLITE3_INCLUDE_DIR}, lib: ${SQLITE3_LIBRARIES}")
message("  libmysmartgrid include: ${LIBMYSMARTGRID_INCLUDE_DIR}, lib: ${LIBMYSMARTGRID_LIBRARY}")
message("  libhexabus include: ${HXB_INCLUDE_DIR}, lib: ${HXB_LIBRARIES}")
