# -*- mode: cmake; -*-
project(hexanode-webfrontend)
cmake_minimum_required(VERSION "2.8" FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/../../cmake_modules ${CMAKE_MODULE_PATH})
include(HexabusCommon)

function (get_revision_info REV_INFO)
  execute_process (
    COMMAND git rev-list origin/master..HEAD
    COMMAND wc -l
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _out_git
    ERROR_VARIABLE _err_git
    RESULT_VARIABLE _res_git
    )
  if (${_res_git} EQUAL 0)
    string (STRIP "${_out_git}" _out_git)
    set (${REV_INFO} ${_out_git} PARENT_SCOPE)
  endif()
endfunction()
get_revision_info (PROJECT_REVISION)
set(HXB_VERSION "${V_MAJOR}.${V_MINOR}.${V_PATCH}.${PROJECT_REVISION}")


add_custom_target (pack_dir 
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
install(DIRECTORY public DESTINATION /home/hexanode/)
install(DIRECTORY tools DESTINATION /home/hexanode/)
install(DIRECTORY views DESTINATION /home/hexanode/)
install(DIRECTORY ${CMAKE_BINARY_DIR}/views DESTINATION /home/hexanode/)

INSTALL(PROGRAMS etc/service/hexanode-webfrontend/run DESTINATION /etc/service/hexanode-webfrontend)
INSTALL(PROGRAMS etc/service/hexanode-webfrontend/log/run DESTINATION /etc/service/hexanode-webfrontend/log)

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

