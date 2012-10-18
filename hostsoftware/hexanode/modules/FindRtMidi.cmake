# - Find RTMIDI
# Find the native RTMIDI includes and library
#
#  RTMIDI_INCLUDE_DIR    - where to find .h
#  RTMIDI_LIBRARIES   - List of libraries when using RTMIDI.
#  RTMIDI_FOUND       - True if RTMIDI found.

if (RTMIDI_INCLUDE_DIR)
  # Already in cache, be silent
  set (RTMIDI_FIND_QUIETLY TRUE)
endif (RTMIDI_INCLUDE_DIR)

find_path (RTMIDI_INCLUDE_DIR RtMidi.h)

find_library (RTMIDI_LIBRARIES NAMES librtmidi.so)

# handle the QUIETLY and REQUIRED arguments and set RTMIDI_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (RTMIDI DEFAULT_MSG RTMIDI_LIBRARIES RTMIDI_INCLUDE_DIR)

mark_as_advanced (RTMIDI_LIBRARIES RTMIDI_INCLUDE_DIR)
