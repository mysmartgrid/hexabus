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

##
SET(_hexabus_HOME "")
if( "${HEXABUS_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{HEXABUS_HOME}")
    message(STATUS "HEXABUS_HOME env is not set, setting it to /usr/local")
    set (HEXABUS_HOME ${_hexabus_HOME})
  else("" MATCHES "$ENV{HEXABUS_HOME}")
    set (HEXABUS_HOME "$ENV{HEXABUS_HOME}")
  endif("" MATCHES "$ENV{HEXABUS_HOME}")
else( "${HEXABUS_HOME}" STREQUAL "")
  message(STATUS "HEXABUS_HOME is not empty: \"${HEXABUS_HOME}\"")
endif( "${HEXABUS_HOME}" STREQUAL "")

##
IF( NOT ${HEXABUS_HOME} STREQUAL "" )
    SET(_hexabus_INCLUDE_SEARCH_DIRS ${HEXABUS_HOME}/include ${_hexabus_INCLUDE_SEARCH_DIRS})
    SET(_hexabus_LIBRARIES_SEARCH_DIRS ${HEXABUS_HOME}/lib ${_hexabus_LIBRARIES_SEARCH_DIRS})
    SET(_hexabus_HOME ${HEXABUS_HOME})
ENDIF( NOT ${HEXABUS_HOME} STREQUAL "" )

message(${_hexabus_INCLUDE_SEARCH_DIRS})

SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
##
find_path (HXB_INCLUDE_DIR libhexabus/common.hpp
  HINTS
  ${_hexabus_INCLUDE_SEARCH_DIRS}
  )

find_library (HXB_LIBRARIES NAMES libhexabus.a
  HINTS
  ${_hexabus_LIBRARIES_SEARCH_DIRS}
)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# handle the QUIETLY and REQUIRED arguments and set HXB_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (HXB DEFAULT_MSG HXB_LIBRARIES HXB_INCLUDE_DIR)

mark_as_advanced (HXB_LIBRARIES HXB_INCLUDE_DIR)
