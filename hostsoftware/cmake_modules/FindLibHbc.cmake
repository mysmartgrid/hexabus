# -*- mode: cmake; -*-
# - Find HBC
# Find the native HBC includes and library
#
#  HBC_INCLUDE_DIR    - where to find .h
#  HBC_LIBRARIES   - List of libraries when using HBC.
#  HBC_FOUND       - True if HBC found.

if (HBC_INCLUDE_DIR)
  # Already in cache, be silent
  set (HBC_FIND_QUIETLY TRUE)
endif (HBC_INCLUDE_DIR)

#
# set defaults
SET(_hbc_HOME "/usr/local")
SET(_hbc_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_hbc_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
SET(_hbc_HOME "")
if( "${HBC_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{HBC_HOME}")
    message(STATUS "HBC_HOME env is not set, setting it to /usr/local")
    set (HBC_HOME ${_hbc_HOME})
  else("" MATCHES "$ENV{HBC_HOME}")
    set (HBC_HOME "$ENV{HBC_HOME}")
  endif("" MATCHES "$ENV{HBC_HOME}")
else( "${HBC_HOME}" STREQUAL "")
  message(STATUS "HBC_HOME is not empty: \"${HBC_HOME}\"")
endif( "${HBC_HOME}" STREQUAL "")

##
IF( NOT ${HBC_HOME} STREQUAL "" )
    SET(_hbc_INCLUDE_SEARCH_DIRS ${HBC_HOME}/include ${_hbc_INCLUDE_SEARCH_DIRS})
    SET(_hbc_LIBRARIES_SEARCH_DIRS ${HBC_HOME}/lib ${_hbc_LIBRARIES_SEARCH_DIRS})
    SET(_hbc_HOME ${HBC_HOME})
ENDIF( NOT ${HBC_HOME} STREQUAL "" )

IF( NOT $ENV{HBC_INCLUDEDIR} STREQUAL "" )
  SET(_hbc_INCLUDE_SEARCH_DIRS $ENV{HBC_INCLUDEDIR} ${_hbc_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{HBC_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{HBC_LIBRARYDIR} STREQUAL "" )
  SET(_hbc_LIBRARIES_SEARCH_DIRS $ENV{HBC_LIBRARYDIR} ${_hbc_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{HBC_LIBRARYDIR} STREQUAL "" )

IF( HBC_HOME )
  SET(_hbc_INCLUDE_SEARCH_DIRS ${HBC_HOME}/include ${_hbc_INCLUDE_SEARCH_DIRS})
  SET(_hbc_LIBRARIES_SEARCH_DIRS ${HBC_HOME}/lib ${_hbc_LIBRARIES_SEARCH_DIRS})
  SET(_hbc_HOME ${HBC_HOME})
ENDIF( HBC_HOME )

message(${_hbc_INCLUDE_SEARCH_DIRS})


find_path (HBC_INCLUDE_DIR libhbc/common.hpp
  HINTS
  ${_hbc_INCLUDE_SEARCH_DIRS}
  ${PC_HBC_INCLUDEDIR}
  ${PC_HBC_INCLUDE_DIRS}
  ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(HBC_LIBRARY_NAMES ${HBC_LIBRARY_NAMES} libhbc.lib)
ELSE(WIN32)
  SET(HBC_LIBRARY_NAMES ${HBC_LIBRARY_NAMES} libhbc.a)
ENDIF(WIN32)
find_library(HBC_LIBRARIES NAMES ${HBC_LIBRARY_NAMES}
  HINTS
    ${_hbc_LIBRARIES_SEARCH_DIRS}
    ${PC_HBC_LIBDIR}
    ${PC_HBC_LIBRARY_DIRS}
)

# handle the QUIETLY and REQUIRED arguments and set HBC_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (HBC DEFAULT_MSG HBC_LIBRARIES HBC_INCLUDE_DIR)

# if the include and the program are found then we have it
if(HBC_INCLUDE_DIR AND HBC_LIBRARY)
  set(HBC_FOUND "YES")
endif()


mark_as_advanced (
  HBC_FOUND
  HBC_LIBRARIES
  HBC_INCLUDE_DIR
  )
