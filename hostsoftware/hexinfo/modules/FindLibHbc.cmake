# - Find HBC
# Find the native HBC includes and library
#
#  HBC_INCLUDE_DIR    - where to find .h
#  HBC_LIBRARIES   - List of libraries when using HXB.
#  HBC_FOUND       - True if HXB found.

if (HBC_INCLUDE_DIR)
  # Already in cache, be silent
  set (HBC_FIND_QUIETLY TRUE)
endif (HBC_INCLUDE_DIR)

find_path (HBC_INCLUDE_DIR libhbc/common.hpp)

find_library (HBC_LIBRARIES NAMES libhbc.a)

# handle the QUIETLY and REQUIRED arguments and set HXB_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (HBC DEFAULT_MSG HBC_LIBRARIES HBC_INCLUDE_DIR)

mark_as_advanced (HBC_LIBRARIES HBC_INCLUDE_DIR)
