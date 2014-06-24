# -*- mode: cmake; -*-
# Revision 1391, 1.2 kB  (checked in by rexbron) 
# - Try to find SQLITE3
# Once done this will define
#
#  SQLITE3_FOUND - system has SQLITE3
#  SQLITE3_INCLUDE_DIR - the SQLITE3 include directory
#  SQLITE3_LIBRARIES - Link these to use SQLITE3
#  SQLITE3_DEFINITIONS - Compiler switches required for using SQLITE3
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if ( SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARIES )
   # in cache already
   SET(SQLITE3_FIND_QUIETLY TRUE)
endif ( SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARIES )

message(STATUS "FindSqlite3 check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (PC_SQLITE3 sqlite3>=0.9)

     set(SQLITE3_DEFINITIONS ${PC_SQLITE3_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_sqlite3_HOME "/usr/local")
SET(_sqlite3_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_sqlite3_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${SQLITE3_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{SQLITE3_HOME}")
    message(STATUS "SQLITE3_HOME env is not set, setting it to /usr/local")
    set (SQLITE3_HOME ${_sqlite3_HOME})
  else("" MATCHES "$ENV{SQLITE3_HOME}")
    set (SQLITE3_HOME "$ENV{SQLITE3_HOME}")
  endif("" MATCHES "$ENV{SQLITE3_HOME}")
else( "${SQLITE3_HOME}" STREQUAL "")
  message(STATUS "SQLITE3_HOME is not empty: \"${SQLITE3_HOME}\"")
endif( "${SQLITE3_HOME}" STREQUAL "")
##

message(STATUS "Looking for sqlite3 in ${SQLITE3_HOME}")

IF( NOT ${SQLITE3_HOME} STREQUAL "" )
    SET(_sqlite3_INCLUDE_SEARCH_DIRS ${SQLITE3_HOME}/include ${_sqlite3_INCLUDE_SEARCH_DIRS})
    SET(_sqlite3_LIBRARIES_SEARCH_DIRS ${SQLITE3_HOME}/lib ${_sqlite3_LIBRARIES_SEARCH_DIRS})
    SET(_sqlite3_HOME ${SQLITE3_HOME})
ENDIF( NOT ${SQLITE3_HOME} STREQUAL "" )

IF( NOT $ENV{SQLITE3_INCLUDEDIR} STREQUAL "" )
  SET(_sqlite3_INCLUDE_SEARCH_DIRS $ENV{SQLITE3_INCLUDEDIR} ${_sqlite3_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{SQLITE3_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{SQLITE3_LIBRARYDIR} STREQUAL "" )
  SET(_sqlite3_LIBRARIES_SEARCH_DIRS $ENV{SQLITE3_LIBRARYDIR} ${_sqlite3_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{SQLITE3_LIBRARYDIR} STREQUAL "" )

IF( SQLITE3_HOME )
  SET(_sqlite3_INCLUDE_SEARCH_DIRS ${SQLITE3_HOME}/include ${_sqlite3_INCLUDE_SEARCH_DIRS})
  SET(_sqlite3_LIBRARIES_SEARCH_DIRS ${SQLITE3_HOME}/lib ${_sqlite3_LIBRARIES_SEARCH_DIRS})
  SET(_sqlite3_HOME ${SQLITE3_HOME})
ENDIF( SQLITE3_HOME )

message("Sqlite3 search: '${_sqlite3_INCLUDE_SEARCH_DIRS}' ${CMAKE_INCLUDE_PATH}")
# find the include files
FIND_PATH(SQLITE3_INCLUDE_DIR sqlite3.h
   HINTS
     ${_sqlite3_INCLUDE_SEARCH_DIRS}
     ${PC_SQLITE3_INCLUDEDIR}
     ${PC_SQLITE3_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(SQLITE3_LIBRARY_NAMES ${SQLITE3_LIBRARY_NAMES} libsqlite3.lib)
ELSE(WIN32)
  SET(SQLITE3_LIBRARY_NAMES ${SQLITE3_LIBRARY_NAMES} libsqlite3.a)
ENDIF(WIN32)
FIND_LIBRARY(SQLITE3_LIBRARIES NAMES ${SQLITE3_LIBRARY_NAMES}
  HINTS
    ${_sqlite3_LIBRARIES_SEARCH_DIRS}
    ${PC_SQLITE3_LIBDIR}
    ${PC_SQLITE3_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARIES)
  SET(SQLITE3_FOUND "YES")
ENDIF(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARIES)

#if( NOT WIN32)
#  list(APPEND SQLITE3_LIBRARY "-lrt")
#endif( NOT WIN32)

MARK_AS_ADVANCED(
  SQLITE3_FOUND
  SQLITE3_LIBRARIES
  SQLITE3_INCLUDE_DIR
)
