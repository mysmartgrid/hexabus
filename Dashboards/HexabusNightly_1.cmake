# -*- mode: cmake; -*-
set(TARGET_ARCH ar71xx)
set(CTEST_PUSH_PACKAGES 1)
get_filename_component(_currentDir "${CMAKE_CURRENT_LIST_FILE}" PATH)
include( "${_currentDir}/HexabusNightly.cmake")
