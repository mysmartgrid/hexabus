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

find_path (CPPNETLIB_INCLUDE_DIR boost/network/version.hpp)

find_library ( CPPNETLIB_CLIENT_CON
  NAMES libcppnetlib-client-connections.a)

find_library ( CPPNETLIB_SERVER_PARSER
  NAMES libcppnetlib-server-parsers.a)

find_library ( CPPNETLIB_URI
  NAMES libcppnetlib-uri.a)

set(CPPNETLIB_LIBRARIES 
  ${CPPNETLIB_CLIENT_CON} 
  ${CPPNETLIB_SERVER_PARSER} 
  ${CPPNETLIB_URI} 
  )

# handle the QUIETLY and REQUIRED arguments and set CPPNETLIB_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (CPPNETLIB DEFAULT_MSG CPPNETLIB_LIBRARIES CPPNETLIB_INCLUDE_DIR)

mark_as_advanced (CPPNETLIB_LIBRARIES CPPNETLIB_INCLUDE_DIR)
