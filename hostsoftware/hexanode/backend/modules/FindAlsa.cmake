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

find_path (ALSA_INCLUDE_DIR RtMidi.h)

find_library (ALSA_LIBRARIES NAMES libasound.so)

# handle the QUIETLY and REQUIRED arguments and set ALSA_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (ALSA DEFAULT_MSG ALSA_LIBRARIES ALSA_INCLUDE_DIR)

mark_as_advanced (ALSA_LIBRARIES ALSA_INCLUDE_DIR)
