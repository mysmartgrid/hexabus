# -*- mode: cmake; -*-
# bernd.loerwald@itwm.fraunhofer.de
# update by kai.krueger@itwm.fraunhofer.de

include(Tools.cmake)
FindOS(OS_NAME OS_VERSION)

# cdash / ctest information ########################################################

set(CTEST_PROJECT_NAME "hexabus")
set(CTEST_NIGHTLY_START_TIME "20:00:00 GMT")

set(CTEST_DROP_METHOD     "http")
set(CTEST_DROP_SITE       "cdash.hexabus.de")
set(CTEST_DROP_LOCATION   "/submit.php?project=${CTEST_PROJECT_NAME}")
set(CTEST_DROP_SITE_CDASH TRUE)
set(CTEST_USE_LAUNCHERS   0)
set(CTEST_TEST_TIMEOUT   180)

set(CTEST_PACKAGE_SITE "msgrid@packages.mysmartgrid.de")

site_name(CTEST_SITE)

# test configuration ###############################################################
# parse arguments
string (REPLACE "," ";" SCRIPT_ARGUMENTS "${CTEST_SCRIPT_ARG}")
foreach (ARGUMENT ${SCRIPT_ARGUMENTS})
  if ("${ARGUMENT}" MATCHES "^([^=]+)=(.+)$" )
    set ("${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
  endif()
endforeach()

if (NOT DEBUG)
  set(doSubmit 1)
else()
  set(doSubmit 0)
endif()

if (NOT FORCE_CONTINUOUS)
  set(FORCE_CONTINUOUS 0)
endif()

if (NOT TESTING_MODEL)
  message (FATAL_ERROR "No TESTING_MODEL given (available: Nightly, Coverage)")
endif()

if (NOT GIT_BRANCH)
  set (GIT_BRANCH "develop")
endif()

if (NOT BOOST_VERSION)
  set (BOOST_VERSION "1.49")
endif()

if (NOT COMPILER)
  set (COMPILER "gcc")
endif()

if (NOT PARALLEL_JOBS)
  set (PARALLEL_JOBS 1)
endif()

if (NOT REPOSITORY_URL)
  set (REPOSITORY_URL "git_gpispace:gpispace.git")
endif ()

if (NOT BUILD_TMP_DIR)
  set (BUILD_TMP_DIR "/tmp/$ENV{USER}")
endif ()

if (${COMPILER} STREQUAL "gcc")
  set (CMAKE_C_COMPILER "gcc")
  set (CMAKE_CXX_COMPILER "g++")
elseif( ${COMPILER} STREQUAL "clang" )
  set (CMAKE_C_COMPILER "clang")
  set (CMAKE_CXX_COMPILER "clang++")
elseif (${COMPILER} STREQUAL "intel")
  set (CMAKE_C_COMPILER "icc")
  set (CMAKE_CXX_COMPILER "icpc")
else()
  message (FATAL_ERROR "unknown compiler '${COMPILER}'")
endif()
find_program( COMPILER_CC ${CMAKE_C_COMPILER} )
find_program( COMPILER_CXX ${CMAKE_CXX_COMPILER} )
if( ${COMPILER_CC} STREQUAL "COMPILER_CC-NOTFOUND" OR ${COMPILER_CXX} STREQUAL "COMPILER_CXX-NOTFOUND")
  message(FATAL_ERROR "Compiler not found. Stopping here.")
  return(1)
endif()

set (ENV{CC} ${CMAKE_C_COMPILER})
set (ENV{CXX} ${CMAKE_CXX_COMPILER})

if(CMAKE_TOOLCHAIN_FILE)
  include(${CMAKE_TOOLCHAIN_FILE})
  if( openwrt_arch ) 
    set(CMAKE_SYSTEM_PROCESSOR ${openwrt_arch})
  endif()
endif()

# variables / configuration based on test configuration ############################
set(_projectNameDir "${CTEST_PROJECT_NAME}")

if(BOOST_VERSION)
  set(_boost_str "-boost${BOOST_VERSION}")
else()
  set(_boost_str "")
endif()
if(CMAKE_COMPILER_VERSION)
  set(_compiler_str "${COMPILER}${CMAKE_COMPILER_VERSION}")
else()
  set(_compiler_str "${COMPILER}")
endif()

set (CTEST_BUILD_NAME "${CMAKE_SYSTEM_PROCESSOR}-${_compiler_str}${_boost_str}-${GIT_BRANCH}")

set (CTEST_BASE_DIRECTORY   "${BUILD_TMP_DIR}/${CTEST_PROJECT_NAME}/${TESTING_MODEL}")
set (CTEST_SOURCE_DIRECTORY "${CTEST_BASE_DIRECTORY}/src-${GIT_BRANCH}-${CMAKE_SYSTEM_PROCESSOR}" )
set (CTEST_BINARY_DIRECTORY "${CTEST_BASE_DIRECTORY}/build-${CTEST_BUILD_NAME}")
set (CTEST_INSTALL_DIRECTORY "${CTEST_BASE_DIRECTORY}/install-${CTEST_BUILD_NAME}")

if (${TESTING_MODEL} STREQUAL "Nightly")
  set (CMAKE_BUILD_TYPE "Release")
elseif (${TESTING_MODEL} STREQUAL "Continuous")
  set (CMAKE_BUILD_TYPE "Release")
elseif (${TESTING_MODEL} STREQUAL "Coverage")
  set (CMAKE_BUILD_TYPE "Profile")
  set (ENABLE_CODECOVERAGE 1)

  find_program (CTEST_COVERAGE_COMMAND NAMES gcov)
  find_program (CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
else()
  message (FATAL_ERROR "Unknown TESTING_MODEL ${TESTING_MODEL} (available: Nightly, Coverage, Continuous)")
endif()

# find and submit subproject list ##################################################
set(CTEST_PROJECT_SUBPROJECTS)
list(APPEND CTEST_PROJECT_SUBPROJECTS "libhexabus")
#file(GLOB _dummy ${CTEST_SOURCE_DIRECTORY}/hostsoftware/*/CMakeLists.txt)
#foreach ( line ${_dummy})
#  get_filename_component(_currentDir "${line}" PATH)
#  get_filename_component(_item "${_currentDir}" NAME)
#  message("====>${line}<====${_currentDir}===${_item}")
#  list(APPEND CTEST_PROJECT_SUBPROJECTS ${_item})
#endforeach()
list(APPEND CTEST_PROJECT_SUBPROJECTS "hba")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hbc")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hexinfo")
list(APPEND CTEST_PROJECT_SUBPROJECTS "network-autoconfig")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hexanode")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hexanode-webfrontend")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hexadaemon")

# create project.xml
set(projectFile "${CTEST_BINARY_DIRECTORY}/Project.xml")
file(WRITE ${projectFile}  "<Project name=\"${CTEST_PROJECT_NAME}\">
")
  
foreach(subproject ${CTEST_PROJECT_SUBPROJECTS})
  file(APPEND ${projectFile}
      "<SubProject name=\"${subproject}\"></SubProject>
")
endforeach()
file(APPEND ${projectFile}
    "</Project>
")
if(doSubmit)
  ctest_submit(FILES "${CTEST_BINARY_DIRECTORY}/Project.xml") 
endif()

set(URL "https://github.com/mysmartgrid/hexabus.git")

# external software ################################################################

if (${COMPILER} STREQUAL "intel")
  set (ADDITIONAL_SUFFIX "intel")
else()
  set (ADDITIONAL_SUFFIX "gcc")
endif()

macro (set_if_exists NAME PATH)
  if (EXISTS ${PATH})
    set (${NAME} ${PATH})
    set ($ENV{Name} ${PATH})
    message("====> Path ${NAME}=${PATH} found.")
  else()
    message("====> Path ${PATH} does not exists.")
  endif()
endmacro()


if( NOT CMAKE_TOOLCHAIN_FILE )
  set (EXTERNAL_SOFTWARE "$ENV{HOME}/external_software")
  set_if_exists (BOOST_ROOT ${EXTERNAL_SOFTWARE}/boost/${BOOST_VERSION})
  set_if_exists (CPPNETLIB_HOME ${EXTERNAL_SOFTWARE}/boost/${BOOST_VERSION}/)
  set_if_exists (GRAPHVIZ_HOME ${EXTERNAL_SOFTWARE}/graphviz/2.24)
  set_if_exists (ALSA_HOME ${EXTERNAL_SOFTWARE}/alsa/1.0.25)
else()
  message("=== Cross env Name: ${CrossName}")
  set_if_exists (EXTERNAL_SOFTWARE "${_baseDir}/opt")
  set_if_exists (BOOST_ROOT ${EXTERNAL_SOFTWARE}/boost/${BOOST_VERSION})
  set_if_exists (ALSA_HOME ${_baseDir}/usr)
  set_if_exists (CPPNETLIB_HOME ${EXTERNAL_SOFTWARE}/cpp-netlib_boost/${BOOST_VERSION}/)

endif()
set_if_exists (LIBKLIO_HOME "${BUILD_TMP_DIR}/libklio/${TESTING_MODEL}/install-${CTEST_BUILD_NAME}")


# cmake options ####################################################################

set (CTEST_BUILD_FLAGS "-k -j ${PARALLEL_JOBS}")
set (CTEST_CMAKE_GENERATOR "Unix Makefiles")


# prepare binary directory (clear, write initial cache) ############################

if (NOT "${TESTING_MODEL}" STREQUAL "Continuous")
  file (REMOVE_RECURSE "${CTEST_INSTALL_DIRECTORY}")
endif()
ctest_empty_binary_directory ("${CTEST_BINARY_DIRECTORY}")

if (NOT EXISTS "${CTEST_BINARY_DIRECTORY}")
  file (MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}")
endif()

# prepare source directory (do initial checkout, switch branch) ####################
find_program (CTEST_GIT_COMMAND NAMES git)

if (NOT EXISTS ${CTEST_SOURCE_DIRECTORY}/.git)
  set (CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone -b ${GIT_BRANCH} ${URL} ${CTEST_SOURCE_DIRECTORY}")
  set (first_checkout 1)
else()
  set (first_checkout 0)
endif()
if(FORCE_CONTINUOUS)
  set (first_checkout 1)
endif()

# do testing #######################################################################

set (UPDATE_RETURN_VALUE 0)

ctest_start (${TESTING_MODEL})

ctest_update (RETURN_VALUE UPDATE_RETURN_VALUE)
message("Update returned: ${UPDATE_RETURN_VALUE}")

if ("${TESTING_MODEL}" STREQUAL "Continuous" AND first_checkout EQUAL 0 AND FORCE_CONTINUOUS EQUAL 0)
  if (UPDATE_RETURN_VALUE EQUAL 0)
    return()
  endif ()
endif ()

# start loop over all subprojects  #################################################
foreach(subproject ${CTEST_PROJECT_SUBPROJECTS})
  message("====>  build ${subproject}")
  set_property(GLOBAL PROPERTY SubProject ${subproject})
  set_property (GLOBAL PROPERTY Label ${subproject})
  set (CMAKE_INSTALL_PREFIX "/usr")

  set_if_exists (HXB_HOME ${CTEST_INSTALL_DIRECTORY})
  set_if_exists (HBC_HOME ${CTEST_INSTALL_DIRECTORY})
  if(${subproject} STREQUAL "hexanode")
    set(CTEST_SUBPROJECT_SOURCE_DIR  ${CTEST_SOURCE_DIRECTORY}/hostsoftware/${subproject}/backend )
  else()
    set(CTEST_SUBPROJECT_SOURCE_DIR  ${CTEST_SOURCE_DIRECTORY}/hostsoftware/${subproject} )
  endif()
  if(${subproject} STREQUAL "hexanode-webfrontend")
    set(CTEST_SUBPROJECT_SOURCE_DIR  ${CTEST_SOURCE_DIRECTORY}/hostsoftware/hexanode/webfrontend)
  endif()

  # write CMakeCache file here
  file (WRITE "${CTEST_BINARY_DIRECTORY}/${subproject}/CMakeCache.txt"
    "# Automatically generated in ctest script (write_initial_cache())\n\n")

  foreach (VARIABLE_NAME
      CMAKE_BUILD_TYPE
      CMAKE_INSTALL_PREFIX
      CMAKE_TOOLCHAIN_FILE
      CMAKE_CXX_COMPILER
      CMAKE_C_COMPILER
      CMAKE_SYSTEM_PROCESSOR
      #    OS_NAME
      #    OS_VERSION
      

      CTEST_TIMEOUT
      CTEST_USE_LAUNCHERS

      ENABLE_CODECOVERAGE


      LIBKLIO_HOME
      HXB_HOME
      HBC_HOME
      CPPNETLIB_HOME 
      ALSA_HOME 

      BOOST_ROOT
      GRAPHVIZ_HOME

      )
    if (DEFINED ${VARIABLE_NAME})
      file (APPEND "${CTEST_BINARY_DIRECTORY}/${subproject}/CMakeCache.txt" "${VARIABLE_NAME}:STRING=${${VARIABLE_NAME}}\n")
    endif()
  endforeach()
  foreach (VARIABLE_NAME
      CMAKE_ADDITIONAL_PATH
      )
    if (DEFINED ${VARIABLE_NAME})
      file (APPEND "${CTEST_BINARY_DIRECTORY}/${subproject}/CMakeCache.txt" "${VARIABLE_NAME}:PATH=${${VARIABLE_NAME}}\n")
    endif()
  endforeach()

  set(CONFIGURE_RETURN_VALUE 0)
  if( EXISTS ${CTEST_SUBPROJECT_SOURCE_DIR} )
    file(MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}/${subproject}")

    ctest_configure (BUILD ${CTEST_BINARY_DIRECTORY}/${subproject} 
      SOURCE ${CTEST_SUBPROJECT_SOURCE_DIR} 
      APPEND RETURN_VALUE CONFIGURE_RETURN_VALUE
      )

    if( STAGING_DIR)
      include(${CTEST_BINARY_DIRECTORY}/${subproject}/CMakeCache.txt)
      set(ENV{STAGING_DIR}     ${OPENWRT_STAGING_DIR})
    endif( STAGING_DIR)

    if (${CONFIGURE_RETURN_VALUE} EQUAL 0)
      ctest_build (BUILD ${CTEST_BINARY_DIRECTORY}/${subproject} 
	APPEND  RETURN_VALUE BUILD_RETURN_VALUE
	NUMBER_ERRORS BUILD_ERRORS
	)

      message("======> run Install:  <===")
      execute_process(
	COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CTEST_INSTALL_DIRECTORY} -P cmake_install.cmake
	WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}/${subproject} 
	RESULT_VARIABLE INSTALL_ERRORS
	)
      message("======> Install: ${INSTALL_ERRORS} <===")

      if (${BUILD_ERRORS} EQUAL 0 AND ${INSTALL_ERRORS} EQUAL 0)
	set (PROPERLY_BUILT_AND_INSTALLED TRUE)
      endif()

      if (${TESTING_MODEL} STREQUAL "Coverage")
	ctest_coverage (RETURN_VALUE LAST_RETURN_VALUE)
	ctest_memcheck (RETURN_VALUE LAST_RETURN_VALUE)
      endif()

      if( NOT CMAKE_TOOLCHAIN_FILE )
	if (PROPERLY_BUILT_AND_INSTALLED)
	  ctest_test (BUILD "${CTEST_BINARY_DIRECTORY}"
	    INCLUDE_LABEL "${subproject}"
	    SCHEDULE_RANDOM true RETURN_VALUE LAST_RETURN_VALUE PARALLEL_LEVEL ${PARALLEL_JOBS})
	else()
	  ctest_test (BUILD "${CTEST_BINARY_DIRECTORY}"
	    INCLUDE_LABEL "${subproject}"
	    SCHEDULE_RANDOM true EXCLUDE_LABEL "requires_installation"
	    RETURN_VALUE LAST_RETURN_VALUE)
	endif()
      endif( NOT CMAKE_TOOLCHAIN_FILE )
    endif()

    if(doSubmit)
      ctest_submit (RETURN_VALUE ${LAST_RETURN_VALUE})
    endif()

    # todo: create package, upload to distribution server
    # do the packing only if switch is on and
    if( (${UPDATE_RETURN_VALUE} GREATER 0 AND PROPERLY_BUILT_AND_INSTALLED) OR ${first_checkout})
      include(${CTEST_BINARY_DIRECTORY}/${subproject}/CPackConfig.cmake)
      if( STAGING_DIR)
	set(ENV{PATH}            ${OPENWRT_STAGING_DIR}/host/bin:$ENV{PATH})
      endif( STAGING_DIR)
      # do the packaging
      execute_process(
	COMMAND cpack -G DEB
	WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}/${subproject}
	RESULT_VARIABLE COMMAND_ERRORS
	)
      message("cpack returned: ${COMMAND_ERRORS}")

      # upload files
      if( ${CTEST_PUSH_PACKAGES})
	message( "OS_NAME .....: ${OS_NAME}")
	message( "OS_VERSION ..: ${OS_VERSION}")
	message( "CMAKE_SYSTEM_PROCESSOR ..: ${CMAKE_SYSTEM_PROCESSOR}")

	if(CPACK_ARCHITECTUR)
	  set(OPKG_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_ARCHITECTUR}")
	  set(_package_file "${OPKG_FILE_NAME}.ipk")
	else(CPACK_ARCHITECTUR)
	  set(_package_file "${CPACK_PACKAGE_FILE_NAME}.deb")
	endif(CPACK_ARCHITECTUR)
	message("==> Upload packages - ${_package_file}")
	set(_export_host ${CTEST_PACKAGE_SITE})
	set(_remote_dir "packages/${OS_NAME}/${OS_VERSION}/${CMAKE_SYSTEM_PROCESSOR}")
	if( NOT ${GIT_BRANCH} STREQUAL "master")
	  set(_remote_dir "packages/${OS_NAME}/${OS_VERSION}/${CMAKE_SYSTEM_PROCESSOR}/${GIT_BRANCH}")
	endif()
	message("Execute: ssh ${_export_host} mkdir -p ${_remote_dir}")
	execute_process(
	  COMMAND ssh ${_export_host} mkdir -p ${_remote_dir}
	  )
	message("Execute scp -p ${_package_file} ${_export_host}:${_remote_dir}/${_package_file}")
	execute_process(
	  COMMAND scp -p ${_package_file} ${_export_host}:${_remote_dir}/${_package_file}
	  WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}/${subproject}
	  RESULT_VARIABLE COMMAND_ERRORS
	  )
	message("scp returned: ${COMMAND_ERRORS}")
      endif()

    endif()

  else()
    message("====>Skipping ${subproject}")
  endif()

endforeach()
# end loop over all subprojects  ###################################################

#file (REMOVE_RECURSE "${CTEST_INSTALL_DIRECTORY}")
#if (NOT "${TESTING_MODEL}" STREQUAL "Continuous")
#  file (REMOVE_RECURSE "${CTEST_BINARY_DIRECTORY}")
#endif()

return (${LAST_RETURN_VALUE})
