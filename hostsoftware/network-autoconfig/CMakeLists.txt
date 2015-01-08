# -*- mode: cmake; -*-
project(hxb-net-autoconfig)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})
include(HexabusCommon)

###################################################################################################
# No user settings below
###################################################################################################

# add some files to the installation target
#INSTALL(FILES README.md LICENSE.txt DESTINATION share/doc/hexaswitch)
INSTALL(PROGRAMS ipaddress.py hxb-net-autoconf.py DESTINATION lib/hxb-net-autoconf)
INSTALL(PROGRAMS hxb-net-autoconf DESTINATION bin)

# CPACK packaging
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "DEB" "STGZ" "TGZ" "TBZ2")
set(CPACK_PACKAGE_VENDOR "Fraunhofer ITWM")
set(CPACK_PACKAGE_CONTACT "Fraunhofer ITWM <support@mysmartgrid.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hexabus RasPi network autoconfiguration")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${V_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${V_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${V_PATCH})

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3 (>= 3.2.3), radvd, mrd6")

set(CPACK_SECTION "net")
include(CPackTools)

### print some output for the user
message("")
message("***** Configuration *****")
message("  prefix: ${CMAKE_INSTALL_PREFIX}")
message("  extended logging: ${ENABLE_LOGGING}")
