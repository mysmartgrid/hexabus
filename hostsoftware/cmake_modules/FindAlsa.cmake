# -*- mode: cmake; -*-
# - Find ALSA
# Find the native ALSA includes and library
#
#  ALSA_INCLUDE_DIR - where to find .h
#  ALSA_LIBRARIES   - List of libraries when using Alsa.
#  ALSA_FOUND       - True if Alsa found.

if (ALSA_INCLUDE_DIR)
  # Already in cache, be silent
  set (ALSA_FIND_QUIETLY TRUE)
endif (ALSA_INCLUDE_DIR)

IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules (PC_ALSA alsa)
    set(ALSA_DEFINITIONS ${PC_ALSA_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_alsa_HOME "/usr/local")
SET(_alsa_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_alsa_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${ALSA_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{ALSA_HOME}")
    message(STATUS "ALSA_HOME env is not set, setting it to /usr/local")
    set (ALSA_HOME ${_alsa_HOME})
  else("" MATCHES "$ENV{ALSA_HOME}")
    set (ALSA_HOME "$ENV{ALSA_HOME}")
  endif("" MATCHES "$ENV{ALSA_HOME}")
endif( "${ALSA_HOME}" STREQUAL "")
message(STATUS "Looking for alsa in ${ALSA_HOME}")

##
IF( NOT ${ALSA_HOME} STREQUAL "" )
    SET(_alsa_INCLUDE_SEARCH_DIRS ${ALSA_HOME}/include ${_alsa_INCLUDE_SEARCH_DIRS})
    SET(_alsa_LIBRARIES_SEARCH_DIRS ${ALSA_HOME}/lib ${_alsa_LIBRARIES_SEARCH_DIRS})
    SET(_alsa_HOME ${ALSA_HOME})
ENDIF( NOT ${ALSA_HOME} STREQUAL "" )

IF( NOT $ENV{ALSA_INCLUDEDIR} STREQUAL "" )
  SET(_alsa_INCLUDE_SEARCH_DIRS $ENV{ALSA_INCLUDEDIR} ${_alsa_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{ALSA_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{ALSA_LIBRARYDIR} STREQUAL "" )
  SET(_alsa_LIBRARIES_SEARCH_DIRS $ENV{ALSA_LIBRARYDIR} ${_alsa_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{ALSA_LIBRARYDIR} STREQUAL "" )

IF( ALSA_HOME )
  SET(_alsa_INCLUDE_SEARCH_DIRS ${ALSA_HOME}/include ${_alsa_INCLUDE_SEARCH_DIRS})
  SET(_alsa_LIBRARIES_SEARCH_DIRS ${ALSA_HOME}/lib ${_alsa_LIBRARIES_SEARCH_DIRS})
  SET(_alsa_HOME ${ALSA_HOME})
ENDIF( ALSA_HOME )

message(STATUS "root_path: ${CMAKE_FIND_ROOT_PATH}")


find_path (ALSA_INCLUDE_DIR alsa/asoundlib.h
  HINTS
     ${_alsa_INCLUDE_SEARCH_DIRS}
     ${PC_ALSA_INCLUDEDIR}
     ${PC_ALSA_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

find_library (ALSA_STATIC_LIBRARIES NAMES libasound.a
  HINTS
    ${_alsa_LIBRARIES_SEARCH_DIRS}
    ${PC_ALSA_LIBDIR}
    ${PC_ALSA_LIBRARY_DIRS}
  )
find_library (ALSA_SHARED_LIBRARIES NAMES libasound.so
  HINTS
    ${_alsa_LIBRARIES_SEARCH_DIRS}
    ${PC_ALSA_LIBDIR}
    ${PC_ALSA_LIBRARY_DIRS}
  )

if(NOT ${ALSA_STATIC_LIBRARIES} STREQUAL "ALSA_STATIC_LIBRARIES-NOTFOUND")
  set(ALSA_LIBRARIES ${ALSA_STATIC_LIBRARIES})
else()
  set(ALSA_LIBRARIES ${ALSA_SHARED_LIBRARIES})
endif()

# handle the QUIETLY and REQUIRED arguments and set ALSA_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (ALSA DEFAULT_MSG ALSA_LIBRARIES  ALSA_INCLUDE_DIR)

# if the include and the program are found then we have it
IF(ALSA_INCLUDE_DIR AND ALSA_LIBRARIES)
  SET(ALSA_FOUND "YES")
ENDIF()

mark_as_advanced (
  ALSA_FOUND
  ALSA_LIBRARIES
  ALSA_INCLUDE_DIR
  )
