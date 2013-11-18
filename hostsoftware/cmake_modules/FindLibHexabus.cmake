# -*- mode: cmake; -*-
# - Find HXB
# Find the native HXB includes and library
#
#  HXB_INCLUDE_DIR    - where to find .h
#  HXB_LIBRARIES   - List of libraries when using HXB.
#  HXB_FOUND       - True if HXB found.

if (HXB_INCLUDE_DIR)
  # Already in cache, be silent
  set (HXB_FIND_QUIETLY TRUE)
endif (HXB_INCLUDE_DIR)

#
# set defaults
SET(_hxb_HOME "/usr/local")
SET(_hxb_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_hxb_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
SET(_hxb_HOME "")
if( "${HXB_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{HXB_HOME}")
    message(STATUS "HXB_HOME env is not set, setting it to /usr/local")
    set (HXB_HOME ${_hxb_HOME})
  else("" MATCHES "$ENV{HXB_HOME}")
    set (HXB_HOME "$ENV{HXB_HOME}")
  endif("" MATCHES "$ENV{HXB_HOME}")
else( "${HXB_HOME}" STREQUAL "")
  message(STATUS "HXB_HOME is not empty: \"${HXB_HOME}\"")
endif( "${HXB_HOME}" STREQUAL "")

##
IF( NOT ${HXB_HOME} STREQUAL "" )
    SET(_hxb_INCLUDE_SEARCH_DIRS ${HXB_HOME}/include ${_hxb_INCLUDE_SEARCH_DIRS})
    SET(_hxb_LIBRARIES_SEARCH_DIRS ${HXB_HOME}/lib ${_hxb_LIBRARIES_SEARCH_DIRS})
    SET(_hxb_HOME ${HXB_HOME})
ENDIF( NOT ${HXB_HOME} STREQUAL "" )

IF( NOT $ENV{HXB_INCLUDEDIR} STREQUAL "" )
  SET(_hxb_INCLUDE_SEARCH_DIRS $ENV{HXB_INCLUDEDIR} ${_hxb_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{HXB_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{HXB_LIBRARYDIR} STREQUAL "" )
  SET(_hxb_LIBRARIES_SEARCH_DIRS $ENV{HXB_LIBRARYDIR} ${_hxb_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{HXB_LIBRARYDIR} STREQUAL "" )

IF( HXB_HOME )
  SET(_hxb_INCLUDE_SEARCH_DIRS ${HXB_HOME}/include ${_hxb_INCLUDE_SEARCH_DIRS})
  SET(_hxb_LIBRARIES_SEARCH_DIRS ${HXB_HOME}/lib ${_hxb_LIBRARIES_SEARCH_DIRS})
  SET(_hxb_HOME ${HXB_HOME})
ENDIF( HXB_HOME )

message(${_hxb_INCLUDE_SEARCH_DIRS})


find_path (HXB_INCLUDE_DIR libhexabus/common.hpp
  HINTS
  ${_hxb_INCLUDE_SEARCH_DIRS}
  ${PC_HXB_INCLUDEDIR}
  ${PC_HXB_INCLUDE_DIRS}
  ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(HXB_LIBRARY_NAMES ${HXB_LIBRARY_NAMES} libhexabus.lib)
ELSE(WIN32)
  SET(HXB_LIBRARY_NAMES ${HXB_LIBRARY_NAMES} libhexabus.a)
ENDIF(WIN32)
find_library(HXB_LIBRARIES NAMES ${HXB_LIBRARY_NAMES}
  HINTS
    ${_hxb_LIBRARIES_SEARCH_DIRS}
    ${PC_HXB_LIBDIR}
    ${PC_HXB_LIBRARY_DIRS}
)

# handle the QUIETLY and REQUIRED arguments and set HXB_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (HXB DEFAULT_MSG HXB_LIBRARIES HXB_INCLUDE_DIR)

# if the include and the program are found then we have it
if(HXB_INCLUDE_DIR AND HXB_LIBRARY)
  set(HXB_FOUND "YES")
endif()


mark_as_advanced (
  HXB_FOUND
  HXB_LIBRARIES
  HXB_INCLUDE_DIR
  )
