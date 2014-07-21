# -*- mode: cmake; -*-

function (get_revision_hash PROJECT_REVISION)
  execute_process (
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _out_git
    ERROR_VARIABLE _err_git
    RESULT_VARIABLE _res_git
    )
  if (${_res_git} EQUAL 0)
    string (STRIP "${_out_git}" _out_git)
    set (${PROJECT_REVISION} ${_out_git} PARENT_SCOPE)
  else()
    set (${PROJECT_REVISION} "REVISION-NOTFOUND" PARENT_SCOPE)
  endif()
endfunction()

get_revision_hash (PROJECT_REVISION_HASH)
if (${PROJECT_REVISION_HASH} EQUAL "REVISION-NOTFOUND")
  message (FATAL_ERROR "could not discover revision info, please define -DPROJECT_REVISION_HASH by hand!")
endif ()
set (PROJECT_REVISION_HASH "${PROJECT_REVISION_HASH}" CACHE STRING "Source code revision" FORCE)
