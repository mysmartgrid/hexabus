# -*- mode: cmake; -*-
project(hexanode-webfrontend)
cmake_minimum_required(VERSION "2.8" FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../../cmake_modules ${CMAKE_MODULE_PATH})
include(HexabusCommon)

###################################################################################################
# No user settings below
###################################################################################################

# Where are the additional libraries installed? Note: provide includes
# path here, subsequent checks will resolve everything else

add_custom_target (pack_dir
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/pack
	COMMAND mkdir -p  ${CMAKE_BINARY_DIR}/pack
	COMMAND cp -r *.js *.json lib public views ${CMAKE_BINARY_DIR}/pack/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )

add_custom_target (node_modules ALL
	COMMAND npm install
	COMMAND npm dedup
    DEPENDS pack_dir
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pack )

install(DIRECTORY ${CMAKE_BINARY_DIR}/pack/node_modules DESTINATION /home/hexanode/)

configure_file(views/about.ejs.in ${CMAKE_BINARY_DIR}/views/about.ejs)
install(FILES app.js DESTINATION /home/hexanode/)
install(FILES package.json DESTINATION /home/hexanode/)
install(FILES npm-shrinkwrap.json DESTINATION /home/hexanode/)
install(DIRECTORY lib DESTINATION /home/hexanode/)
install(DIRECTORY controllers DESTINATION /home/hexanode/)
install(DIRECTORY statemachines DESTINATION /home/hexanode/)
install(DIRECTORY public DESTINATION /home/hexanode/)
install(DIRECTORY tools DESTINATION /home/hexanode/)
install(DIRECTORY views DESTINATION /home/hexanode/)
install(DIRECTORY ${CMAKE_BINARY_DIR}/views DESTINATION /home/hexanode/)

INSTALL(PROGRAMS etc/service/hexanode-webfrontend/run DESTINATION /etc/service/hexanode-webfrontend)
INSTALL(PROGRAMS etc/service/hexanode-webfrontend/log/run DESTINATION /etc/service/hexanode-webfrontend/log)
INSTALL(PROGRAMS etc/service/hexanode-hexajuice/run DESTINATION /etc/service/hexanode-hexajuice)
INSTALL(PROGRAMS etc/service/hexanode-hexajuice/log/run DESTINATION /etc/service/hexanode-hexajuice/log)


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

set(CPACK_DEBIAN_PACKAGE_DEPENDS "nodejs (>= 0.10), npm (>= 1.2), socat, daemontools")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm;${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")

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

