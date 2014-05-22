# -*- mode: cmake; -*-
# - Try to find libuci include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * UCI_FOUND if protoc was found
# * UCI_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * UCI_INCLUDE The include directories for libuci.

message(STATUS "FindUci check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (PC_UCI uci)

     set(UCI_DEFINITIONS ${PC_UCI_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_uci_HOME "/usr/local")
SET(_uci_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_uci_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${UCI_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{UCI_HOME}")
    message(STATUS "UCI_HOME env is not set, setting it to /usr/local")
    set (UCI_HOME ${_uci_HOME})
  else("" MATCHES "$ENV{UCI_HOME}")
    set (UCI_HOME "$ENV{UCI_HOME}")
  endif("" MATCHES "$ENV{UCI_HOME}")
else( "${UCI_HOME}" STREQUAL "")
  message(STATUS "UCI_HOME is not empty: \"${UCI_HOME}\"")
endif( "${UCI_HOME}" STREQUAL "")
##

message(STATUS "Looking for uci in ${UCI_HOME}")

IF( NOT ${UCI_HOME} STREQUAL "" )
    SET(_uci_INCLUDE_SEARCH_DIRS ${UCI_HOME}/include ${_uci_INCLUDE_SEARCH_DIRS})
    SET(_uci_LIBRARIES_SEARCH_DIRS ${UCI_HOME}/lib ${_uci_LIBRARIES_SEARCH_DIRS})
    SET(_uci_HOME ${UCI_HOME})
ENDIF( NOT ${UCI_HOME} STREQUAL "" )

IF( NOT $ENV{UCI_INCLUDEDIR} STREQUAL "" )
  SET(_uci_INCLUDE_SEARCH_DIRS $ENV{UCI_INCLUDEDIR} ${_uci_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{UCI_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{UCI_LIBRARYDIR} STREQUAL "" )
  SET(_uci_LIBRARIES_SEARCH_DIRS $ENV{UCI_LIBRARYDIR} ${_uci_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{UCI_LIBRARYDIR} STREQUAL "" )

IF( UCI_HOME )
  SET(_uci_INCLUDE_SEARCH_DIRS ${UCI_HOME}/include ${_uci_INCLUDE_SEARCH_DIRS})
  SET(_uci_LIBRARIES_SEARCH_DIRS ${UCI_HOME}/lib ${_uci_LIBRARIES_SEARCH_DIRS})
  SET(_uci_HOME ${UCI_HOME})
ENDIF( UCI_HOME )

message("UCI search: '${_uci_INCLUDE_SEARCH_DIRS}' ${CMAKE_INCLUDE_PATH}")
# find the include files
FIND_PATH(UCI_INCLUDE_DIR uci.h
   HINTS
     ${_uci_INCLUDE_SEARCH_DIRS}
     ${PC_UCI_INCLUDEDIR}
     ${PC_UCI_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(UCI_LIBRARY_NAMES ${UCI_LIBRARY_NAMES} libuci.lib)
ELSE(WIN32)
  SET(UCI_LIBRARY_NAMES ${UCI_LIBRARY_NAMES} libuci.a)
ENDIF(WIN32)
FIND_LIBRARY(UCI_LIBRARY NAMES ${UCI_LIBRARY_NAMES}
  HINTS
    ${_uci_LIBRARIES_SEARCH_DIRS}
    ${PC_UCI_LIBDIR}
    ${PC_UCI_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(UCI_INCLUDE_DIR AND UCI_LIBRARY)
  SET(UCI_FOUND "YES")
ENDIF(UCI_INCLUDE_DIR AND UCI_LIBRARY)

if( NOT WIN32)
  list(APPEND UCI_LIBRARY "-ldl")
endif( NOT WIN32)

MARK_AS_ADVANCED(
  UCI_FOUND
  UCI_LIBRARY
  UCI_INCLUDE_DIR
)
