# -*- mode: cmake; -*-
# - Try to find libjsoncpp include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * JSONCPP_FOUND if protoc was found
# * JSONCPP_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * JSONCPP_INCLUDE The include directories for libjsoncpp.

message(STATUS "FindJsonCpp check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (PC_JSONCPP jsoncpp)

     set(JSONCPP_DEFINITIONS ${PC_JSONCPP_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_jsoncpp_HOME "/usr/local")
SET(_jsoncpp_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_jsoncpp_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${JSONCPP_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{JSONCPP_HOME}")
    message(STATUS "JSONCPP_HOME env is not set, setting it to /usr/local")
    set (JSONCPP_HOME ${_jsoncpp_HOME})
  else("" MATCHES "$ENV{JSONCPP_HOME}")
    set (JSONCPP_HOME "$ENV{JSONCPP_HOME}")
  endif("" MATCHES "$ENV{JSONCPP_HOME}")
else( "${JSONCPP_HOME}" STREQUAL "")
  message(STATUS "JSONCPP_HOME is not empty: \"${JSONCPP_HOME}\"")
endif( "${JSONCPP_HOME}" STREQUAL "")
##

message(STATUS "Looking for jsoncpp in ${JSONCPP_HOME}")

IF( NOT ${JSONCPP_HOME} STREQUAL "" )
    SET(_jsoncpp_INCLUDE_SEARCH_DIRS ${JSONCPP_HOME}/include ${_jsoncpp_INCLUDE_SEARCH_DIRS})
    SET(_jsoncpp_LIBRARIES_SEARCH_DIRS ${JSONCPP_HOME}/lib ${_jsoncpp_LIBRARIES_SEARCH_DIRS})
    SET(_jsoncpp_HOME ${JSONCPP_HOME})
ENDIF( NOT ${JSONCPP_HOME} STREQUAL "" )

IF( NOT $ENV{JSONCPP_INCLUDEDIR} STREQUAL "" )
  SET(_jsoncpp_INCLUDE_SEARCH_DIRS $ENV{JSONCPP_INCLUDEDIR} ${_jsoncpp_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{JSONCPP_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{JSONCPP_LIBRARYDIR} STREQUAL "" )
  SET(_jsoncpp_LIBRARIES_SEARCH_DIRS $ENV{JSONCPP_LIBRARYDIR} ${_jsoncpp_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{JSONCPP_LIBRARYDIR} STREQUAL "" )

IF( JSONCPP_HOME )
  SET(_jsoncpp_INCLUDE_SEARCH_DIRS ${JSONCPP_HOME}/include ${_jsoncpp_INCLUDE_SEARCH_DIRS})
  SET(_jsoncpp_LIBRARIES_SEARCH_DIRS ${JSONCPP_HOME}/lib ${_jsoncpp_LIBRARIES_SEARCH_DIRS})
  SET(_jsoncpp_HOME ${JSONCPP_HOME})
ENDIF( JSONCPP_HOME )

message("Jsoncpp search: '${_jsoncpp_INCLUDE_SEARCH_DIRS}' ${CMAKE_INCLUDE_PATH}")
# find the include files
FIND_PATH(JSONCPP_INCLUDE_DIR jsoncpp/json/json.h
   HINTS
     ${_jsoncpp_INCLUDE_SEARCH_DIRS}
     ${PC_JSONCPP_INCLUDEDIR}
     ${PC_JSONCPP_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
SET(JSONCPP_LIBRARY_NAMES ${JSONCPP_LIBRARY_NAMES} jsoncpp)
FIND_LIBRARY(JSONCPP_LIBRARY NAMES ${JSONCPP_LIBRARY_NAMES}
  HINTS
    ${_jsoncpp_LIBRARIES_SEARCH_DIRS}
    ${PC_JSONCPP_LIBDIR}
    ${PC_JSONCPP_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARY)
  SET(JSONCPP_FOUND "YES")
ENDIF(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARY)

#if( NOT WIN32)
#  list(APPEND JSONCPP_LIBRARY "-lrt")
#endif( NOT WIN32)

MARK_AS_ADVANCED(
  JSONCPP_FOUND
  JSONCPP_LIBRARY
  JSONCPP_INCLUDE_DIR
)
