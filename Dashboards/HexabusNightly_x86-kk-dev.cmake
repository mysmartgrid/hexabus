# set(TARGET_ARCH ar71xx)
set(_git_branch "kk-dev")
get_filename_component(_currentDir "${CMAKE_CURRENT_LIST_FILE}" PATH)
include( "${_currentDir}/HexabusNightly.cmake")
