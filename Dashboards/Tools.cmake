# -*- mode: cmake; -*-

function(ctest_push_files packageDir distFile)

  set(_export_host "192.168.9.63")
  execute_process(
    COMMAND scp -p ${distFile} ${_export_host}:packages
    WORKING_DIRECTORY ${packageDir}
    )
endfunction(ctest_push_files)
