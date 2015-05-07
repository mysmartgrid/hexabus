# -*- mode: cmake; -*-
# bernd.loerwald@itwm.fraunhofer.de
# update by kai.krueger@itwm.fraunhofer.de
get_filename_component(_currentDir "${CMAKE_CURRENT_LIST_FILE}" PATH)

include(${_currentDir}/Tools.cmake)
include(${_currentDir}/CTestGIT.cmake)
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
set(CTEST_CONTINUOUS_RUN_TIME 36000)
set(CTEST_CONTINUOUS_SLEEP   120)

set(CTEST_PACKAGE_SITE "msgrid@packages.mysmartgrid.de")

site_name(CTEST_SITE)

my_ctest_setup()

# find and submit subproject list ##################################################
set(CTEST_PROJECT_SUBPROJECTS)
list(APPEND CTEST_PROJECT_SUBPROJECTS "libhexabus")
#list(APPEND CTEST_PROJECT_SUBPROJECTS "hba")
#list(APPEND CTEST_PROJECT_SUBPROJECTS "hbc")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hbt")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hexinfo")
list(APPEND CTEST_PROJECT_SUBPROJECTS "network-autoconfig")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hexanode")
list(APPEND CTEST_PROJECT_SUBPROJECTS "hexanode-webfrontend")

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
    set (ENV{${NAME}} ${PATH})
    message("====> Path ${NAME}=${PATH} found.")
  else()
    message("====> Path '${PATH}' does not exists.")
  endif()
endmacro()


if( NOT CMAKE_TOOLCHAIN_FILE )
  set (EXTERNAL_SOFTWARE "$ENV{HOME}/external_software")
  set_if_exists (BOOST_ROOT ${EXTERNAL_SOFTWARE}/boost/${BOOST_VERSION})
  set_if_exists (CPPNETLIB_HOME ${EXTERNAL_SOFTWARE}/boost/${BOOST_VERSION}/)
  set_if_exists (GRAPHVIZ_HOME ${EXTERNAL_SOFTWARE}/graphviz/2.24)
  set_if_exists (ALSA_HOME ${EXTERNAL_SOFTWARE}/alsa/1.0.25)
  set_if_exists (SQLITE3_HOME ${EXTERNAL_SOFTWARE}/sqlite/3.8.5)
else()
  message("=== Cross env Name: ${CrossName}")
  set_if_exists (EXTERNAL_SOFTWARE "${_baseDir}/opt")
  set_if_exists (BOOST_ROOT ${EXTERNAL_SOFTWARE}/boost/${BOOST_VERSION})
  set_if_exists (ALSA_HOME ${_baseDir}/usr)
  set_if_exists (CLN_HOME ${_baseDir}/usr)
  set_if_exists (CPPNETLIB_HOME ${EXTERNAL_SOFTWARE}/cpp-netlib_boost/${BOOST_VERSION}/)
  set_if_exists (SQLITE3_HOME ${EXTERNAL_SOFTWARE}/sqlite/3.8.5)

endif()
set_if_exists (LIBKLIO_HOME "${BUILD_TMP_DIR}/libklio/${TESTING_MODEL}/install-${CTEST_BUILD_NAME_BASE}")
if(NOT LIBKLIO_HOME)
  set_if_exists (LIBKLIO_HOME "${BUILD_TMP_DIR}/libklio/${TESTING_MODEL}/install-${CTEST_BUILD_NAME_DEVEL}")
endif()
if(NOT LIBKLIO_HOME)
  set_if_exists (LIBKLIO_HOME "${BUILD_TMP_DIR}/libklio/${TESTING_MODEL}/install-${CTEST_BUILD_NAME_MASTER}")
endif()
if(NOT LIBKLIO_HOME)
  set_if_exists (LIBKLIO_HOME "${BUILD_TMP_DIR}/libklio/Nightly/install-${CTEST_BUILD_NAME_DEVEL}")
endif()

set_if_exists (LIBMYSMARTGRID_HOME "${BUILD_TMP_DIR}/libmysmartgrid/${TESTING_MODEL}/install-${CTEST_BUILD_NAME_BASE}")
if(NOT LIBMYSMARTGRID_HOME)
  set_if_exists (LIBMYSMARTGRID_HOME "${BUILD_TMP_DIR}/libmysmartgrid/${TESTING_MODEL}/install-${CTEST_BUILD_NAME_DEVEL}")
endif()
if(NOT LIBMYSMARTGRID_HOME)
  set_if_exists (LIBMYSMARTGRID_HOME "${BUILD_TMP_DIR}/libmysmartgrid/${TESTING_MODEL}/install-${CTEST_BUILD_NAME_MASTER}")
endif()
if(NOT LIBMYSMARTGRID_HOME)
  set_if_exists (LIBMYSMARTGRID_HOME "${BUILD_TMP_DIR}/libmysmartgrid/Nightly/install-${CTEST_BUILD_NAME_DEVEL}")
endif()


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

message("check for sourcedir: {CTEST_SOURCE_DIRECTORY}")
if (NOT EXISTS ${CTEST_SOURCE_DIRECTORY} AND NOT EXISTS ${CTEST_SOURCE_DIRECTORY}/.git)
  #set (CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone -b ${GIT_BRANCH} ${URL} ${CTEST_SOURCE_DIRECTORY}")
  message("===> clone repository")
  git_clone(${GIT_BRANCH} ${URL})
  set (first_checkout 1)
else()
  set (first_checkout 0)
endif()
if(FORCE_CONTINUOUS)
  set (first_checkout 1)
endif()

# do testing #######################################################################
set (UPDATE_RETURN_VALUE 0)
set(firstLoop TRUE)

message("========== ========== RUN BUILD ========== ==========")
message("Running next loop...")
set(firstLoop 1)

if("${TESTING_MODEL}" STREQUAL "Continuous")

  while (${CTEST_ELAPSED_TIME} LESS ${CTEST_CONTINUOUS_RUN_TIME})
    message("========== ========== RUN BUILD ========== ==========")
    message("Running next loop...")

    git_fetch()
    git_revision_list(revision_list)
    message("===========> Revlist: ${revision_list}")

    if(firstLoop) 
      ctest_start (${TESTING_MODEL})
      ctest_update (SOURCE ${CTEST_SOURCE_DIRECTORY} RETURN_VALUE UPDATE_RETURN_VALUE)
      message("Update returned: ${UPDATE_RETURN_VALUE}")
      run_build()
      set(firstLoop FALSE)
    endif()

    foreach(rev ${revision_list})
      message("Build revision ${rev}")
      set(CTEST_GIT_UPDATE_CUSTOM ${CTEST_GIT_COMMAND} merge ${rev})

      ctest_start (${TESTING_MODEL})
      ctest_update (SOURCE ${CTEST_SOURCE_DIRECTORY} RETURN_VALUE UPDATE_RETURN_VALUE)
      message("Update returned: ${UPDATE_RETURN_VALUE}")

      if ("${UPDATE_RETURN_VALUE}" GREATER 0  OR  firstLoop OR first_checkout)
	run_build()
	set(firstLoop FALSE)
	set(first_checkout 0)
      endif()
    endforeach()
    ctest_sleep( 120 )
  endwhile()
else()
endif()

return (${LAST_RETURN_VALUE})
