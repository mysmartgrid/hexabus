# -*- mode: cmake; -*-
# - Try to find libklio include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * LIBKLIO_FOUND if protoc was found
# * LIBKLIO_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * LIBKLIO_INCLUDE The include directories for libklio.

cmake_policy(PUSH)
# when crosscompiling import the executable targets from a file
if(CMAKE_CROSSCOMPILING)
#  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY FIRST)
#  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE FIRST)
endif(CMAKE_CROSSCOMPILING)


message(STATUS "FindLibklio check")

IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules (PC_LIBKLIO libklio)
    set(LIBKLIO_DEFINITIONS ${PC_LIBKLIO_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_libklio_HOME "/usr/local")
SET(_libklio_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_libklio_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${LIBKLIO_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{LIBKLIO_HOME}")
    message(STATUS "LIBKLIO_HOME env is not set, setting it to /usr/local")
    set (LIBKLIO_HOME ${_libklio_HOME})
  else("" MATCHES "$ENV{LIBKLIO_HOME}")
    set (LIBKLIO_HOME "$ENV{LIBKLIO_HOME}")
  endif("" MATCHES "$ENV{LIBKLIO_HOME}")
endif( "${LIBKLIO_HOME}" STREQUAL "")
message(STATUS "Looking for libklio in ${LIBKLIO_HOME}")

##
IF( NOT ${LIBKLIO_HOME} STREQUAL "" )
    SET(_libklio_INCLUDE_SEARCH_DIRS ${LIBKLIO_HOME}/include ${_libklio_INCLUDE_SEARCH_DIRS})
    SET(_libklio_LIBRARIES_SEARCH_DIRS ${LIBKLIO_HOME}/lib ${_libklio_LIBRARIES_SEARCH_DIRS})
    SET(_libklio_HOME ${LIBKLIO_HOME})
ENDIF( NOT ${LIBKLIO_HOME} STREQUAL "" )

IF( NOT $ENV{LIBKLIO_INCLUDEDIR} STREQUAL "" )
  SET(_libklio_INCLUDE_SEARCH_DIRS $ENV{LIBKLIO_INCLUDEDIR} ${_libklio_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBKLIO_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{LIBKLIO_LIBRARYDIR} STREQUAL "" )
  SET(_libklio_LIBRARIES_SEARCH_DIRS $ENV{LIBKLIO_LIBRARYDIR} ${_libklio_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBKLIO_LIBRARYDIR} STREQUAL "" )

IF( LIBKLIO_HOME )
  SET(_libklio_INCLUDE_SEARCH_DIRS ${LIBKLIO_HOME}/include ${_libklio_INCLUDE_SEARCH_DIRS})
  SET(_libklio_LIBRARIES_SEARCH_DIRS ${LIBKLIO_HOME}/lib ${_libklio_LIBRARIES_SEARCH_DIRS})
  SET(_libklio_HOME ${LIBKLIO_HOME})
ENDIF( LIBKLIO_HOME )

message(STATUS "root_path: ${CMAKE_FIND_ROOT_PATH}")

find_path(LIBKLIO_INCLUDE_DIR libklio/exporter.hpp
  HINTS
     ${_libklio_INCLUDE_SEARCH_DIRS}
     ${PC_LIBKLIO_INCLUDEDIR}
     ${PC_LIBKLIO_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(LIBKLIO_LIBRARY_NAMES ${LIBKLIO_LIBRARY_NAMES} libklio.lib)
ELSE(WIN32)
  SET(LIBKLIO_LIBRARY_NAMES ${LIBKLIO_LIBRARY_NAMES} libklio.a)
ENDIF(WIN32)
find_library(LIBKLIO_LIBRARY NAMES ${LIBKLIO_LIBRARY_NAMES}
  HINTS
    ${_libklio_LIBRARIES_SEARCH_DIRS}
    ${PC_LIBKLIO_LIBDIR}
    ${PC_LIBKLIO_LIBRARY_DIRS}
)

#
message(STATUS "    Found ${_name}: ${LIBKLIO_INCLUDE_DIRS} ${LIBKLIO_LIBRARY}")
get_filename_component (LIBKLIO_LIBRARY_DIR ${LIBKLIO_LIBRARY} PATH)
#get_filename_component (LIBKLIO_LIBRARIES ${LIBKLIO_LIBRARY} NAME)
#set(LIBKLIO_LIBRARY_DIR ${LIBKLIO_LIBRARY_DIR})
#set(LIBKLIO_FOUND ${LIBKLIO_FOUND})

message(STATUS "    LIBKLIO 2: '-I${LIBKLIO_INCLUDE_DIR}' '-L${LIBKLIO_LIBRARY_DIR}' ")
message(STATUS "             '${LIBKLIO_LIBRARIES}' '${LIBKLIO_LIBRARY}'")
message(STATUS "    search '${LIBKLIO_LIBRARY_DIR}/shared/${_name}Config.cmake'")

#
if( NOT libklio_IN_CACHE )
  if(EXISTS ${LIBKLIO_LIBRARY_DIR}/shared/libklioConfig.cmake)
    message(STATUS "    Include LIBKLIO dependencies.")
    include(${LIBKLIO_LIBRARY_DIR}/shared/libklioConfig.cmake)
    set(LIBKLIO_LIBRARY klio )
    set(LIBKLIO_INCLUDE_DIRS ${LIBKLIO_INCLUDE_DIRS} )
  endif(EXISTS ${LIBKLIO_LIBRARY_DIR}/shared/libklioConfig.cmake)
else( NOT libklio_IN_CACHE )
  message(STATUS "    package ${NAME} was allready in loaded. Do not perform dependencies.")
endif( NOT libklio_IN_CACHE )

message(STATUS "    LIBKLIO 2: '${LIBKLIO_INCLUDE_DIRS}' '${LIBKLIO_LIBRARY_DIR}' ")
message(STATUS "           LIBKLIO_LIBRARIES:  '${LIBKLIO_LIBRARIES}'")
message(STATUS "           LIBKLIO_LIBRARY  :  '${LIBKLIO_LIBRARY}'")
message(STATUS "    search '${LIBKLIO_LIBRARY_DIR}/shared/libklioConfig.cmake'")
message("  rocksdb: ${LIBKLIO_ENABLE_ROCKSDB}")
message("  mysmartgrid: ${LIBKLIO_ENABLE_MSG}")


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Libklio  DEFAULT_MSG LIBKLIO_LIBRARY_NAMES LIBKLIO_INCLUDE_DIR)

# if the include and the program are found then we have it
IF(LIBKLIO_INCLUDE_DIR AND LIBKLIO_LIBRARY)
  SET(LIBKLIO_FOUND "YES")
ENDIF(LIBKLIO_INCLUDE_DIR AND LIBKLIO_LIBRARY)

# if( NOT WIN32)
#   list(APPEND LIBKLIO_LIBRARY "-lrt")
# endif( NOT WIN32)

MARK_AS_ADVANCED(
  LIBKLIO_FOUND
  LIBKLIO_LIBRARY
  LIBKLIO_INCLUDE_DIR
)
if(CMAKE_CROSSCOMPILING)
#  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif(CMAKE_CROSSCOMPILING)
cmake_policy(POP)
