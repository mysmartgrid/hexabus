# -*- mode: cmake; -*-
project(hexanode-webfrontend)
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

# Detect the system we're compiling on
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(HAS_MACOS 1)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(HAS_MACOS 0)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(HAS_LINUX 1)
  SET(__LINUX_ALSA__ 1)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(HAS_LINUX 0)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


# add some files to the installation target
install(FILES app.js DESTINATION share/hexanode_webfrontend/)
install(FILES package.json DESTINATION share/hexanode_webfrontend/)
install(DIRECTORY lib DESTINATION share/hexanode_webfrontend/)
install(DIRECTORY public DESTINATION share/hexanode_webfrontend/)
install(DIRECTORY tools DESTINATION share/hexanode_webfrontend/)
install(DIRECTORY views DESTINATION share/hexanode_webfrontend/)

INSTALL(PROGRAMS etc/service/hexanode/run DESTINATION /etc/service/hexanode)
INSTALL(PROGRAMS etc/service/hexanode/log/run DESTINATION /etc/service/hexanode/log)

# CPACK packaging
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A hexabus display device built with nodejs")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})

set(CPACK_DEBIAN_PACKAGE_DEPENDS "nodejs (>= 0.10), npm (>= 1.2)")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")

set(CPACK_PACKAGE_EXECUTABLES "hexanode webfrontend" "A virtual hexabus display device.")
set(CPACK_SOURCE_STRIP_FILES "")
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
message("  extended logging: ${ENABLE_LOGGING}")

