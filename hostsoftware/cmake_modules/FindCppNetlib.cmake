# -*- mode: cmake; -*-
# - Find CPPNETLIB
# Find the native CPPNETLIB includes and library
#
#  CPPNETLIB_INCLUDE_DIR    - where to find .h
#  CPPNETLIB_LIBRARIES   - List of libraries when using CPPNETLIB.
#  CPPNETLIB_FOUND       - True if CPPNETLIB found.

if (CPPNETLIB_INCLUDE_DIR)
  # Already in cache, be silent
  set (CPPNETLIB_FIND_QUIETLY TRUE)
endif (CPPNETLIB_INCLUDE_DIR)


IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules (PC_CPPNETLIB cppnetlib)
    set(CPPNETLIB_DEFINITIONS ${PC_CPPNETLIB_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_cppnetlib_HOME "/usr/local")
SET(_cppnetlib_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_cppnetlib_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${CPPNETLIB_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{CPPNETLIB_HOME}")
    message(STATUS "CPPNETLIB_HOME env is not set, setting it to /usr/local")
    set (CPPNETLIB_HOME ${_cppnetlib_HOME})
  else("" MATCHES "$ENV{CPPNETLIB_HOME}")
    set (CPPNETLIB_HOME "$ENV{CPPNETLIB_HOME}")
  endif("" MATCHES "$ENV{CPPNETLIB_HOME}")
endif( "${CPPNETLIB_HOME}" STREQUAL "")
message(STATUS "Looking for cppnetlib in ${CPPNETLIB_HOME}")

##
IF( NOT ${CPPNETLIB_HOME} STREQUAL "" )
    SET(_cppnetlib_INCLUDE_SEARCH_DIRS ${CPPNETLIB_HOME}/include ${_cppnetlib_INCLUDE_SEARCH_DIRS})
    SET(_cppnetlib_LIBRARIES_SEARCH_DIRS ${CPPNETLIB_HOME}/lib ${_cppnetlib_LIBRARIES_SEARCH_DIRS})
    SET(_cppnetlib_HOME ${CPPNETLIB_HOME})
ENDIF( NOT ${CPPNETLIB_HOME} STREQUAL "" )

IF( NOT $ENV{CPPNETLIB_INCLUDEDIR} STREQUAL "" )
  SET(_cppnetlib_INCLUDE_SEARCH_DIRS $ENV{CPPNETLIB_INCLUDEDIR} ${_cppnetlib_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{CPPNETLIB_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{CPPNETLIB_LIBRARYDIR} STREQUAL "" )
  SET(_cppnetlib_LIBRARIES_SEARCH_DIRS $ENV{CPPNETLIB_LIBRARYDIR} ${_cppnetlib_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{CPPNETLIB_LIBRARYDIR} STREQUAL "" )

IF( CPPNETLIB_HOME )
  SET(_cppnetlib_INCLUDE_SEARCH_DIRS ${CPPNETLIB_HOME}/include ${_cppnetlib_INCLUDE_SEARCH_DIRS})
  SET(_cppnetlib_LIBRARIES_SEARCH_DIRS ${CPPNETLIB_HOME}/lib ${_cppnetlib_LIBRARIES_SEARCH_DIRS})
  SET(_cppnetlib_HOME ${CPPNETLIB_HOME})
ENDIF( CPPNETLIB_HOME )

message(STATUS "root_path: ${CMAKE_FIND_ROOT_PATH}")

find_path(CPPNETLIB_INCLUDE_DIR  boost/network/version.hpp
  HINTS
     ${_cppnetlib_INCLUDE_SEARCH_DIRS}
     ${PC_CPPNETLIB_INCLUDEDIR}
     ${PC_CPPNETLIB_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
find_library(CPPNETLIB_STATIC_CLIENT_CON
  NAMES libcppnetlib-client-connections.a
  HINTS
    ${_cppnetlib_LIBRARIES_SEARCH_DIRS}
    ${PC_CPPNETLIB_LIBDIR}
    ${PC_CPPNETLIB_LIBRARY_DIRS}
)
if(NOT ${CPPNETLIB_STATIC_CLIENT_CON})
  find_library(CPPNETLIB_SHARED_CLIENT_CON
    NAMES libcppnetlib-client-connections.so
    HINTS
    ${_cppnetlib_LIBRARIES_SEARCH_DIRS}
    ${PC_CPPNETLIB_LIBDIR}
    ${PC_CPPNETLIB_LIBRARY_DIRS}
    )
  set(CPPNETLIB_CLIENT_CON ${CPPNETLIB_SHARED_CLIENT_CON})
else()
  set(CPPNETLIB_CLIENT_CON ${CPPNETLIB_STATIC_CLIENT_CON})
endif()

find_library ( CPPNETLIB_STATIC_SERVER_PARSER
  NAMES libcppnetlib-server-parsers.a
  HINTS
    ${_cppnetlib_LIBRARIES_SEARCH_DIRS}
    ${PC_CPPNETLIB_LIBDIR}
    ${PC_CPPNETLIB_LIBRARY_DIRS}
)
if(NOT ${CPPNETLIB_STATIC_SERVER_PARSER})
  find_library ( CPPNETLIB_SHARED_SERVER_PARSER
    NAMES libcppnetlib-server-parsers.so
    HINTS
    ${_cppnetlib_LIBRARIES_SEARCH_DIRS}
    ${PC_CPPNETLIB_LIBDIR}
    ${PC_CPPNETLIB_LIBRARY_DIRS}
    )
  set(CPPNETLIB_SERVER_PARSER ${CPPNETLIB_SHARED_SERVER_PARSER})
else()
  set(CPPNETLIB_SERVER_PARSER ${CPPNETLIB_STATIC_SERVER_PARSER})
endif()

find_library ( CPPNETLIB_STATIC_URI
  NAMES libcppnetlib-uri.a
  HINTS
    ${_cppnetlib_LIBRARIES_SEARCH_DIRS}
    ${PC_CPPNETLIB_LIBDIR}
    ${PC_CPPNETLIB_LIBRARY_DIRS}
)
if(NOT ${CPPNETLIB_STATIC_URI})
  find_library ( CPPNETLIB_SHARED_URI
    NAMES libcppnetlib-uri.so
    HINTS
    ${_cppnetlib_LIBRARIES_SEARCH_DIRS}
    ${PC_CPPNETLIB_LIBDIR}
    ${PC_CPPNETLIB_LIBRARY_DIRS}
    )
  set(CPPNETLIB_URI ${CPPNETLIB_SHARED_URI})
else()
  set(CPPNETLIB_URI ${CPPNETLIB_STATIC_URI})
endif()

set(CPPNETLIB_LIBRARIES 
  ${CPPNETLIB_CLIENT_CON} 
  ${CPPNETLIB_SERVER_PARSER} 
  ${CPPNETLIB_URI} 
  )

# handle the QUIETLY and REQUIRED arguments and set CPPNETLIB_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (CPPNETLIB DEFAULT_MSG CPPNETLIB_LIBRARIES CPPNETLIB_INCLUDE_DIR)

# if the include and the program are found then we have it
if(CPPNETLIB_INCLUDE_DIR AND CPPNETLIB_LIBRARIES)
  set(CPPNETLIB_FOUND "YES")
endif()

mark_as_advanced (
  CPPNETLIB_FOUND
  CPPNETLIB_LIBRARIES
  CPPNETLIB_INCLUDE_DIR
  )
