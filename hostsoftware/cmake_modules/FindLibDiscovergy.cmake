# -*- mode: cmake; -*-
# - Try to find libdiscovergy include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * LIBDISCOVERGY_FOUND if protoc was found
# * LIBDISCOVERGY_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * LIBDISCOVERGY_INCLUDE The include directories for libdiscovergy.

cmake_policy(PUSH)
# when crosscompiling import the executable targets from a file
if(CMAKE_CROSSCOMPILING)
#  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY FIRST)
#  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE FIRST)
endif(CMAKE_CROSSCOMPILING)


message(STATUS "FindLibDiscovergy check")

IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules (PC_LIBDISCOVERGY libdiscovergy)
    set(LIBDISCOVERGY_DEFINITIONS ${PC_LIBDISCOVERGY_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_libdiscovergy_HOME "/usr/local")
SET(_libdiscovergy_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_libdiscovergy_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${LIBDISCOVERGY_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{LIBDISCOVERGY_HOME}")
    message(STATUS "LIBDISCOVERGY_HOME env is not set, setting it to /usr/local")
    set (LIBDISCOVERGY_HOME ${_libdiscovergy_HOME})
  else("" MATCHES "$ENV{LIBDISCOVERGY_HOME}")
    set (LIBDISCOVERGY_HOME "$ENV{LIBDISCOVERGY_HOME}")
  endif("" MATCHES "$ENV{LIBDISCOVERGY_HOME}")
endif( "${LIBDISCOVERGY_HOME}" STREQUAL "")
message(STATUS "Looking for libdiscovergy in ${LIBDISCOVERGY_HOME}")

##
IF( NOT ${LIBDISCOVERGY_HOME} STREQUAL "" )
    SET(_libdiscovergy_INCLUDE_SEARCH_DIRS ${LIBDISCOVERGY_HOME}/include ${_libdiscovergy_INCLUDE_SEARCH_DIRS})
    SET(_libdiscovergy_LIBRARIES_SEARCH_DIRS ${LIBDISCOVERGY_HOME}/lib ${_libdiscovergy_LIBRARIES_SEARCH_DIRS})
    SET(_libdiscovergy_HOME ${LIBDISCOVERGY_HOME})
ENDIF( NOT ${LIBDISCOVERGY_HOME} STREQUAL "" )

IF( NOT $ENV{LIBDISCOVERGY_INCLUDEDIR} STREQUAL "" )
  SET(_libdiscovergy_INCLUDE_SEARCH_DIRS $ENV{LIBDISCOVERGY_INCLUDEDIR} ${_libdiscovergy_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBDISCOVERGY_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{LIBDISCOVERGY_LIBRARYDIR} STREQUAL "" )
  SET(_libdiscovergy_LIBRARIES_SEARCH_DIRS $ENV{LIBDISCOVERGY_LIBRARYDIR} ${_libdiscovergy_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBDISCOVERGY_LIBRARYDIR} STREQUAL "" )

IF( LIBDISCOVERGY_HOME )
  SET(_libdiscovergy_INCLUDE_SEARCH_DIRS ${LIBDISCOVERGY_HOME}/include ${_libdiscovergy_INCLUDE_SEARCH_DIRS})
  SET(_libdiscovergy_LIBRARIES_SEARCH_DIRS ${LIBDISCOVERGY_HOME}/lib ${_libdiscovergy_LIBRARIES_SEARCH_DIRS})
  SET(_libdiscovergy_HOME ${LIBDISCOVERGY_HOME})
ENDIF( LIBDISCOVERGY_HOME )

message(STATUS "root_path: ${CMAKE_FIND_ROOT_PATH}")

find_path(LIBDISCOVERGY_INCLUDE_DIR libdiscovergy/webclient.h
  HINTS
     ${_libdiscovergy_INCLUDE_SEARCH_DIRS}
     ${PC_LIBDISCOVERGY_INCLUDEDIR}
     ${PC_LIBDISCOVERGY_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
SET(LIBDISCOVERGY_LIBRARY_NAMES ${LIBDISCOVERGY_LIBRARY_NAMES} discovergy)
find_library(LIBDISCOVERGY_LIBRARY NAMES ${LIBDISCOVERGY_LIBRARY_NAMES}
  HINTS
    ${_libdiscovergy_LIBRARIES_SEARCH_DIRS}
    ${PC_LIBDISCOVERGY_LIBDIR}
    ${PC_LIBDISCOVERGY_LIBRARY_DIRS}
)

#
message(STATUS "    Found ${_name}: ${LIBDISCOVERGY_INCLUDE_DIRS} ${LIBDISCOVERGY_LIBRARY}")
get_filename_component (LIBDISCOVERGY_LIBRARY_DIR ${LIBDISCOVERGY_LIBRARY} PATH)

if( NOT libdiscovergy_IN_CACHE )
  if(EXISTS ${LIBDISCOVERGY_LIBRARY_DIR}/shared/libdiscovergyConfig.cmake)
    message(STATUS "    Include LIBDISCOVERGY dependencies.")
    include(${LIBDISCOVERGY_LIBRARY_DIR}/shared/libdiscovergyConfig.cmake)
    set(LIBDISCOVERGY_LIBRARY discovergy )
    set(LIBDISCOVERGY_INCLUDE_DIRS ${LIBDISCOVERGY_INCLUDE_DIRS} )
  endif(EXISTS ${LIBDISCOVERGY_LIBRARY_DIR}/shared/libdiscovergyConfig.cmake)
else( NOT libdiscovergy_IN_CACHE )
  message(STATUS "    package ${NAME} was allready in loaded. Do not perform dependencies.")
endif( NOT libdiscovergy_IN_CACHE )

message(STATUS "    LIBDISCOVERGY 2: '-I${LIBDISCOVERGY_INCLUDE_DIR}' '-L${LIBDISCOVERGY_LIBRARY_DIR}' ")
message(STATUS "             '${LIBDISCOVERGY_LIBRARIES}' '${LIBDISCOVERGY_LIBRARY}'")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(libdiscovergy  DEFAULT_MSG LIBDISCOVERGY_LIBRARY_NAMES LIBDISCOVERGY_INCLUDE_DIR)

# if the include and the program are found then we have it
IF(LIBDISCOVERGY_INCLUDE_DIR AND LIBDISCOVERGY_LIBRARY)
  SET(LIBDISCOVERGY_FOUND "YES")
ENDIF(LIBDISCOVERGY_INCLUDE_DIR AND LIBDISCOVERGY_LIBRARY)

# if( NOT WIN32)
#   list(APPEND LIBDISCOVERGY_LIBRARY "-lrt")
# endif( NOT WIN32)

MARK_AS_ADVANCED(
  LIBDISCOVERGY_FOUND
  LIBDISCOVERGY_LIBRARY
  LIBDISCOVERGY_INCLUDE_DIR
  LIBDISCOVERGY_INCLUDE_DIRS
)
if(CMAKE_CROSSCOMPILING)
#  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif(CMAKE_CROSSCOMPILING)
cmake_policy(POP)
