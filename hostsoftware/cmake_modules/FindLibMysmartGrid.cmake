# -*- mode: cmake; -*-
# - Try to find libmysmartgrid include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * LIBMYSMARTGRID_FOUND if protoc was found
# * LIBMYSMARTGRID_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * LIBMYSMARTGRID_INCLUDE The include directories for libmysmartgrid.

cmake_policy(PUSH)
# when crosscompiling import the executable targets from a file
if(CMAKE_CROSSCOMPILING)
#  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY FIRST)
#  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE FIRST)
endif(CMAKE_CROSSCOMPILING)


message(STATUS "FindLibMysmartGrid check")

IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules (PC_LIBMYSMARTGRID libmysmartgrid)
    set(LIBMYSMARTGRID_DEFINITIONS ${PC_LIBMYSMARTGRID_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_libmysmartgrid_HOME "/usr/local")
SET(_libmysmartgrid_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_libmysmartgrid_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${LIBMYSMARTGRID_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{LIBMYSMARTGRID_HOME}")
    message(STATUS "LIBMYSMARTGRID_HOME env is not set, setting it to /usr/local")
    set (LIBMYSMARTGRID_HOME ${_libmysmartgrid_HOME})
  else("" MATCHES "$ENV{LIBMYSMARTGRID_HOME}")
    set (LIBMYSMARTGRID_HOME "$ENV{LIBMYSMARTGRID_HOME}")
  endif("" MATCHES "$ENV{LIBMYSMARTGRID_HOME}")
endif( "${LIBMYSMARTGRID_HOME}" STREQUAL "")
message(STATUS "Looking for libmysmartgrid in ${LIBMYSMARTGRID_HOME}")

##
IF( NOT ${LIBMYSMARTGRID_HOME} STREQUAL "" )
    SET(_libmysmartgrid_INCLUDE_SEARCH_DIRS ${LIBMYSMARTGRID_HOME}/include ${_libmysmartgrid_INCLUDE_SEARCH_DIRS})
    SET(_libmysmartgrid_LIBRARIES_SEARCH_DIRS ${LIBMYSMARTGRID_HOME}/lib ${_libmysmartgrid_LIBRARIES_SEARCH_DIRS})
    SET(_libmysmartgrid_HOME ${LIBMYSMARTGRID_HOME})
ENDIF( NOT ${LIBMYSMARTGRID_HOME} STREQUAL "" )

IF( NOT $ENV{LIBMYSMARTGRID_INCLUDEDIR} STREQUAL "" )
  SET(_libmysmartgrid_INCLUDE_SEARCH_DIRS $ENV{LIBMYSMARTGRID_INCLUDEDIR} ${_libmysmartgrid_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBMYSMARTGRID_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{LIBMYSMARTGRID_LIBRARYDIR} STREQUAL "" )
  SET(_libmysmartgrid_LIBRARIES_SEARCH_DIRS $ENV{LIBMYSMARTGRID_LIBRARYDIR} ${_libmysmartgrid_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBMYSMARTGRID_LIBRARYDIR} STREQUAL "" )

IF( LIBMYSMARTGRID_HOME )
  SET(_libmysmartgrid_INCLUDE_SEARCH_DIRS ${LIBMYSMARTGRID_HOME}/include ${_libmysmartgrid_INCLUDE_SEARCH_DIRS})
  SET(_libmysmartgrid_LIBRARIES_SEARCH_DIRS ${LIBMYSMARTGRID_HOME}/lib ${_libmysmartgrid_LIBRARIES_SEARCH_DIRS})
  SET(_libmysmartgrid_HOME ${LIBMYSMARTGRID_HOME})
ENDIF( LIBMYSMARTGRID_HOME )

message(STATUS "root_path: ${CMAKE_FIND_ROOT_PATH}")

find_path(LIBMYSMARTGRID_INCLUDE_DIR libmysmartgrid/webclient.h
  HINTS
     ${_libmysmartgrid_INCLUDE_SEARCH_DIRS}
     ${PC_LIBMYSMARTGRID_INCLUDEDIR}
     ${PC_LIBMYSMARTGRID_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
SET(LIBMYSMARTGRID_LIBRARY_NAMES ${LIBMYSMARTGRID_LIBRARY_NAMES} mysmartgrid)
find_library(LIBMYSMARTGRID_LIBRARY NAMES ${LIBMYSMARTGRID_LIBRARY_NAMES}
  HINTS
    ${_libmysmartgrid_LIBRARIES_SEARCH_DIRS}
    ${PC_LIBMYSMARTGRID_LIBDIR}
    ${PC_LIBMYSMARTGRID_LIBRARY_DIRS}
)

#
message(STATUS "    Found ${_name}: ${LIBMYSMARTGRID_INCLUDE_DIRS} ${LIBMYSMARTGRID_LIBRARY}")
get_filename_component (LIBMYSMARTGRID_LIBRARY_DIR ${LIBMYSMARTGRID_LIBRARY} PATH)

if( NOT libmysmartgrid_IN_CACHE )
  if(EXISTS ${LIBMYSMARTGRID_LIBRARY_DIR}/shared/libmysmartgridConfig.cmake)
    message(STATUS "    Include LIBMYSMARTGRID dependencies.")
    include(${LIBMYSMARTGRID_LIBRARY_DIR}/shared/libmysmartgridConfig.cmake)
    set(LIBMYSMARTGRID_LIBRARY mysmartgrid )
    set(LIBMYSMARTGRID_INCLUDE_DIRS ${LIBMYSMARTGRID_INCLUDE_DIRS} )
  endif(EXISTS ${LIBMYSMARTGRID_LIBRARY_DIR}/shared/libmysmartgridConfig.cmake)
else( NOT libmysmartgrid_IN_CACHE )
  message(STATUS "    package ${NAME} was allready in loaded. Do not perform dependencies.")
endif( NOT libmysmartgrid_IN_CACHE )

message(STATUS "    LIBMYSMARTGRID 2: '-I${LIBMYSMARTGRID_INCLUDE_DIR}' '-L${LIBMYSMARTGRID_LIBRARY_DIR}' ")
message(STATUS "             '${LIBMYSMARTGRID_LIBRARIES}' '${LIBMYSMARTGRID_LIBRARY}'")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibMysmartGrid  DEFAULT_MSG LIBMYSMARTGRID_LIBRARY_NAMES LIBMYSMARTGRID_INCLUDE_DIR)

# if the include and the program are found then we have it
IF(LIBMYSMARTGRID_INCLUDE_DIR AND LIBMYSMARTGRID_LIBRARY)
  SET(LIBMYSMARTGRID_FOUND "YES")
ENDIF(LIBMYSMARTGRID_INCLUDE_DIR AND LIBMYSMARTGRID_LIBRARY)

# if( NOT WIN32)
#   list(APPEND LIBMYSMARTGRID_LIBRARY "-lrt")
# endif( NOT WIN32)

MARK_AS_ADVANCED(
  LIBMYSMARTGRID_FOUND
  LIBMYSMARTGRID_LIBRARY
  LIBMYSMARTGRID_INCLUDE_DIR
  LIBMYSMARTGRID_INCLUDE_DIRS
)
if(CMAKE_CROSSCOMPILING)
#  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif(CMAKE_CROSSCOMPILING)
cmake_policy(POP)
