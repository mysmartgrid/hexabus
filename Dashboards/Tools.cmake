# -*- mode: cmake; -*-

# generic support code, provides the kde_ctest_setup() macro, which sets up everything required:
get_filename_component(_currentDir "${CMAKE_CURRENT_LIST_FILE}" PATH)

#-----------------------------------------------------------------------------
# Macro my_ctest_setup
macro(my_ctest_setup)
  message("==== run setup =====")
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
    set (GIT_BRANCH "development")
  endif()

  # check for boost and default to version 1.54 if directory does not exists
  if (NOT BOOST_VERSION)
    set (BOOST_VERSION "1.54")
  else()
    if( NOT CMAKE_TOOLCHAIN_FILE )
      set (EXTERNAL_SOFTWARE "$ENV{HOME}/external_software")
      if (EXISTS ${EXTERNAL_SOFTWARE}/boost/${BOOST_VERSION})
	message("====> Path for boost ${BOOST_VERSION} found.")
      else()
	message("====> Path for boost ${BOOST_VERSION} does not exists, defaulting to 1.54")
	set (BOOST_VERSION "1.54")
      endif()
    endif()
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

  if(NOT CMAKE_TOOLCHAIN_FILE)
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
  else()
    set(COMPILER gcc)
  endif()
  message("=========== Compiler: ${COMPILER}")
  message("=========== Compiler: ${COMPILER_CC}")
  message("=========== Compiler: ${COMPILER_CXX}")
  message("=========== Compiler: ${CMAKE_C_COMPILER}")
  message("=========== Compiler: ${CMAKE_CXX_COMPILER}")

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

  set (CTEST_BUILD_NAME_BASE   "${CMAKE_SYSTEM_PROCESSOR}-${_compiler_str}${_boost_str}-${GIT_BRANCH}")
  set (CTEST_BUILD_NAME_DEVEL  "${CMAKE_SYSTEM_PROCESSOR}-${_compiler_str}${_boost_str}-development")
  set (CTEST_BUILD_NAME_MASTER "${CMAKE_SYSTEM_PROCESSOR}-${_compiler_str}${_boost_str}-master")
  set (CTEST_BUILD_NAME        "${CTEST_BUILD_NAME_BASE}")

  set (CTEST_BASE_DIRECTORY   "${BUILD_TMP_DIR}/${CTEST_PROJECT_NAME}/${TESTING_MODEL}")
  set (CTEST_SOURCE_DIRECTORY "${CTEST_BASE_DIRECTORY}/src-${CTEST_BUILD_NAME_BASE}")
  set (CTEST_BINARY_DIRECTORY "${CTEST_BASE_DIRECTORY}/build-${CTEST_BUILD_NAME_BASE}")
  set (CTEST_INSTALL_DIRECTORY "${CTEST_BASE_DIRECTORY}/install-${CTEST_BUILD_NAME_BASE}")

  if (${TESTING_MODEL} STREQUAL "Nightly")
    set (CMAKE_BUILD_TYPE "Release")
  elseif (${TESTING_MODEL} STREQUAL "Continuous")
    set (CMAKE_BUILD_TYPE "Debug")
  elseif (${TESTING_MODEL} STREQUAL "Coverage")
    set (CMAKE_BUILD_TYPE "Profile")
    set (ENABLE_CODECOVERAGE 1)

    find_program (CTEST_COVERAGE_COMMAND NAMES gcov)
    find_program (CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
  else()
    message (FATAL_ERROR "Unknown TESTING_MODEL ${TESTING_MODEL} (available: Nightly, Coverage, Continuous)")
  endif()

endmacro()

#-----------------------------------------------------------------------------
# Macro my_ctest_setup_old
macro(my_ctest_setup_old)
  set(_git_default_branch "development")
  set(_arch "default")

  # init branch to checkout
  if( NOT _git_branch )
    set(_git_branch ${_git_default_branch})
  endif( NOT _git_branch )

  # initialisation target architectur
  if(NOT TARGET_ARCH)
    message("=1======> Setup Arch: ${TARGET_ARCH}")
    set(CTEST_BUILD_ARCH "linux")
  else(NOT TARGET_ARCH)
    message("=2======> Setup Arch: ${TARGET_ARCH}")
    set(CTEST_BUILD_ARCH ${TARGET_ARCH})
    if(${TARGET_ARCH} MATCHES "ar71xx")
      message("=3======> Setup Arch: ${TARGET_ARCH}")
      set(CMAKE_TOOLCHAIN_FILE /usr/local/OpenWrt-SDK-ar71xx-for-Linux-x86_64-gcc-4.3.3+cs_uClibc-0.9.30.1/staging_dir/host/Modules/Toolchain-OpenWRT.cmake)
    else(${TARGET_ARCH} MATCHES "ar71xx")
      set(CMAKE_TOOLCHAIN_FILE "CMAKE_TOOLCHAIN_FILE-NO_FOUND")
    endif(${TARGET_ARCH} MATCHES "ar71xx")
  endif(NOT TARGET_ARCH)

  # check if ToolChainFile is set
  if ( _toolchain_file )
    set(CMAKE_TOOLCHAIN_FILE ${_toolchain_file})
    message("======>  toolchain")
    message("Cross Compiling...${CMAKE_TOOLCHAIN_FILE}")
    get_filename_component(_baseDir "${_toolchain_file}" PATH)
    string(REGEX REPLACE ".*/" "" _toochain ${_baseDir})
    message("Cross Compiling...${_toochain}")
    string(REGEX REPLACE "(.*)-(.*)-(.*)-(.*)" "\\1" _arch ${_toochain})
  endif ( _toolchain_file )

  # initialisation compiler
  if( NOT compiler )
    set(COMPILER_ID "gcc")
  else( NOT compiler )
    if( ${compiler} STREQUAL "clang" )
      find_program( CLANG_CC clang )
      find_program( CLANG_CXX clang++ )
      if( ${CLANG_CC} STREQUAL "CLANG_CC-NOTFOUND" OR ${CLANG_CXX} STREQUAL "CLANG_CC-NOTFOUND")
	message(FATAL_ERROR "clang compiler not found. stopping here.")
      else( ${CLANG_CC} STREQUAL "CLANG_CC-NOTFOUND" OR ${CLANG_CXX} STREQUAL "CLANG_CC-NOTFOUND")
	set(ENV{CC} ${CLANG_CC})
	set(ENV{CXX} ${CLANG_CXX})
	set(COMPILER_ID "clang")
      endif( ${CLANG_CC} STREQUAL "CLANG_CC-NOTFOUND" OR ${CLANG_CXX} STREQUAL "CLANG_CC-NOTFOUND")
    else( ${compiler} STREQUAL "clang" )
      message(FATAL_ERROR "Error. Compiler '${compiler}' not found. Stopping here.")
    endif( ${compiler} STREQUAL "clang" )
  endif( NOT compiler )

  # init branch to checkout
  if( NOT CTEST_PUSH_PACKAGES )
    set(CTEST_PUSH_PACKAGES 0)
  endif( NOT CTEST_PUSH_PACKAGES )

  # setup CTEST_BUILD_NAME
  if( ${_git_branch} STREQUAL ${_git_default_branch} )
    set(CTEST_BUILD_NAME "${CTEST_BUILD_ARCH}-${COMPILER_ID}-${_arch}")
  else( ${_git_branch} STREQUAL ${_git_default_branch} )
    set(CTEST_BUILD_NAME "${CTEST_BUILD_ARCH}-${COMPILER_ID}-${_arch}-${_git_branch}")
  endif( ${_git_branch} STREQUAL ${_git_default_branch} )

  set(_projectNameDir "${CTEST_PROJECT_NAME}")
  set(_srcDir "srcdir")
  set(_buildDir "builddir")

  set(GIT_UPDATE_OPTIONS "pull")

endmacro()

#
#
#
function(create_project_xml) 
  set(projectFile "${CTEST_BINARY_DIRECTORY}/Project.xml")
  file(WRITE ${projectFile}  "<Project name=\"${CTEST_PROJECT_NAME}\">")
  
  foreach(subproject ${CTEST_PROJECT_SUBPROJECTS})
    file(APPEND ${projectFile}
      "<SubProject name=\"${subproject}\">
</SubProject>
")
  endforeach()
  file(APPEND ${projectFile}
    "</Project>
")
  ctest_submit(FILES "${CTEST_BINARY_DIRECTORY}/Project.xml") 
endfunction()

#
#
#
macro(configure_ctest_config _CTEST_VCS_REPOSITORY configfile)
  string(REGEX REPLACE "[ /:\\.]" "_" _tmpDir ${_CTEST_VCS_REPOSITORY})
  set(_tmpDir "${_CTEST_DASHBOARD_DIR}/tmp/${_tmpDir}")
  configure_file(${configfile} ${_tmpDir}/CTestConfig.cmake COPYONLY)
  set(ctest_config ${_tmpDir}/CTestConfig.cmake)
endmacro()

#
#
#
function(FindOS OS_NAME OS_VERSION)

  set(_LinuxReleaseFiles
    redhat-release
    SuSE-release
    lsb-release
    )

  foreach(releasefile ${_LinuxReleaseFiles})
    if(EXISTS "/etc/${releasefile}" )
      file(READ "/etc/${releasefile}" _data )
      if( ${releasefile} MATCHES "redhat-release" )
	ReadRelease_redhat(${_data})# OS_NAME OS_VERSION)
      elseif( ${releasefile} MATCHES "SuSE-release" )
	ReadRelease_SuSE(${_data} OS_NAME OS_VERSION)
      elseif( ${releasefile} MATCHES "lsb-release" )
	ReadRelease_lsb(${_data})# OS_NAME OS_VERSION)
      endif( ${releasefile} MATCHES "redhat-release" )
    endif(EXISTS "/etc/${releasefile}" )
  endforeach()
  #message("  OS_NAME   :  ${OS_NAME}")
  #message("  OS_VERSION:  ${OS_VERSION}")
  set(OS_NAME ${OS_NAME} PARENT_SCOPE)
  set(OS_VERSION ${OS_VERSION} PARENT_SCOPE)
endfunction(FindOS)

macro(ReadRelease_redhat _data OS_NAME OS_VERSION)
  # CentOS release 5.5 (Final)
  set(OS_NAME "CentOS")
  set(OS_NAME "Fedora")
  
endmacro()

macro(ReadRelease_SuSE _data OS_NAME OS_VERSION)
  # openSUSE 11.1 (x86_64)
  # VERSION = 11.1

  set(OS_NAME "openSUSE")
  string(REGEX REPLACE "VERSION=\"(.*)\"" "\\1" OS_VERSION ${_data})

endmacro()

macro(ReadRelease_lsb _data)
  # DISTRIB_ID=Ubuntu
  # DISTRIB_RELEASE=10.04
  # DISTRIB_CODENAME=lucid
  # DISTRIB_DESCRIPTION="Ubuntu 10.04.4 LTS"
  string(REGEX MATCH "DISTRIB_ID=[a-zA-Z0-9]+" _dummy ${_data})
  string(REGEX REPLACE "DISTRIB_ID=(.*)" "\\1" OS_NAME ${_dummy})
  string(REGEX MATCH "DISTRIB_RELEASE=[a-zA-Z0-9.]+" _dummy ${_data})
  string(REGEX REPLACE "DISTRIB_RELEASE=(.*)" "\\1" OS_VERSION ${_dummy})
  #message("    OS_NAME   :  ${OS_NAME}")
  #message("    OS_VERSION:  ${OS_VERSION}")
endmacro()

MARK_AS_ADVANCED(
  OS_NAME
  OS_VERSION
  )

function(ctest_push_files packageDir distFile)

  set(_export_host "192.168.9.63")
  execute_process(
    COMMAND scp -p ${distFile} ${_export_host}:packages
    WORKING_DIRECTORY ${packageDir}
    )
endfunction(ctest_push_files)

#-----------------------------------------------------------------------------
# Function check_Boost
# find the default boost version on the system
function(check_Boost BOOST_VERSION BOOST_LIB_VERSION)
  message("======> check Boost:  <===")
  #//string(REGEX REPLACE "[ /:\\.]" "_" _tmpDir ${_CTEST_VCS_REPOSITORY})
  #//set(_tmpDir "${_CTEST_DASHBOARD_DIR}/tmp/${_tmpDir}")
  set(_tmpDir ${BUILD_TMP_DIR}/blah)
  file (MAKE_DIRECTORY "${_tmpDir}")
  set(_cmakefile ${_tmpDir}/CMakeLists.txt)
  file(WRITE ${_cmakefile}
    "project(testboost)
cmake_minimum_required(VERSION \"2.6\" FATAL_ERROR)

set(CMAKE_MODULE_PATH ${_currentDir}/../modules ${CMAKE_SOURCE_DIR}/../cmake_modules ${CMAKE_MODULE_PATH})
set(Boost_USE_STATIC_LIBS ON)
set(Boost_DETAILED_FAILURE_MSG true)
find_package(Boost 1.46.1 COMPONENTS thread)
"
    )
  execute_process(
    COMMAND ${CMAKE_COMMAND} .
    WORKING_DIRECTORY ${_tmpDir}
    RESULT_VARIABLE INSTALL_ERRORS
    )
  file(READ "${_tmpDir}/CMakeCache.txt" _data )
  string(REGEX MATCH "Boost_LIB_VERSION:INTERNAL=[0-9_]+" _dummy ${_data})

  if( "x${_dummy}" STREQUAL "x")
    set(${BOOST_LIB_VERSION} "0_00" PARENT_SCOPE)
    set(${BOOST_VERSION} "000000" PARENT_SCOPE)
  else()
    string(REGEX MATCH "Boost_LIB_VERSION:INTERNAL=[0-9_]+" _dummy ${_data})
    string(REGEX REPLACE "Boost_LIB_VERSION:INTERNAL=(.*)" "\\1" _BOOST_LIB_VERSION ${_dummy})
    string(REGEX MATCH "Boost_VERSION:INTERNAL=[0-9_]+" _dummy ${_data})
    string(REGEX REPLACE "Boost_VERSION:INTERNAL=(.*)" "\\1" _BOOST_VERSION ${_dummy})
    set(${BOOST_LIB_VERSION} ${_BOOST_LIB_VERSION} PARENT_SCOPE)
    set(${BOOST_VERSION} ${_BOOST_VERSION} PARENT_SCOPE)
  endif()

endfunction()

#-----------------------------------------------------------------------------
# Macro run_build
# build the project submodules
macro(run_build)
  message("========== ========== RUN BUILD ========== ==========")
  # start loop over all subprojects  #################################################
  foreach(subproject ${CTEST_PROJECT_SUBPROJECTS})
    message("====>  build ${subproject}")
    #set (CTEST_BUILD_NAME "${CTEST_BUILD_NAME_BASE}-${subproject}")

    set_property(GLOBAL PROPERTY SubProject ${subproject})
    set_property (GLOBAL PROPERTY Label ${subproject})
    set (CMAKE_INSTALL_PREFIX "/usr")

    set_if_exists (HXB_HOME ${CTEST_INSTALL_DIRECTORY})
    set_if_exists (HBC_HOME ${CTEST_INSTALL_DIRECTORY})
    set_if_exists (HBT_HOME ${CTEST_INSTALL_DIRECTORY})
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
	LIBMYSMARTGRID_HOME
	HXB_HOME
	HBC_HOME
	HBT_HOME
	CPPNETLIB_HOME 
	ALSA_HOME 
	CLN_HOME 
	JSON_HOME 

	BOOST_ROOT
	GRAPHVIZ_HOME
	SQLITE3_HOME

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
	    ctest_test (BUILD "${CTEST_BINARY_DIRECTORY}/${subproject}"
	      #	    INCLUDE_LABEL "${subproject}"
	      SCHEDULE_RANDOM true RETURN_VALUE LAST_RETURN_VALUE PARALLEL_LEVEL ${PARALLEL_JOBS})
	  else()
	    ctest_test (BUILD "${CTEST_BINARY_DIRECTORY}/${subproject}"
	      #	    INCLUDE_LABEL "${subproject}"
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
endmacro()
