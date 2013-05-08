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

find_path (HXB_INCLUDE_DIR libhexabus/common.hpp)

find_library (HXB_LIBRARIES NAMES libhexabus.a)

# handle the QUIETLY and REQUIRED arguments and set HXB_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (HXB DEFAULT_MSG HXB_LIBRARIES HXB_INCLUDE_DIR)

mark_as_advanced (HXB_LIBRARIES HXB_INCLUDE_DIR)
