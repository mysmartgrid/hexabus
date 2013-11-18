# -*- mode: cmake; -*-
#
#  Figure out the version of the used compiler
#  Variables set by this module
#  CMAKE_CXX_COMPILER_MAJOR  major version of compiler
#  CMAKE_CXX_COMPILER_MINR   minor version of compiler
#  CMAKE_CXX_COMPILER_PATCH  patch level (e.g. gcc 4.1.0)
#

#execute_process(COMMAND <cmd1> [args1...]]
#                  [COMMAND <cmd2> [args2...] [...]]
#                  [WORKING_DIRECTORY <directory>]
#                  [TIMEOUT <seconds>]
#                  [RESULT_VARIABLE <variable>]
#                  [OUTPUT_VARIABLE <variable>]
#                  [ERROR_VARIABLE <variable>]
#                  [INPUT_FILE <file>]
#                  [OUTPUT_FILE <file>]
#                  [ERROR_FILE <file>]
#                  [OUTPUT_QUIET]
#                  [ERROR_QUIET]
#                  [OUTPUT_STRIP_TRAILING_WHITESPACE]
#                  [ERROR_STRIP_TRAILING_WHITESPACE])

# check the version of the compiler
set(CMAKE_CXX_COMPILER_MAJOR "CMAKE_CXX_COMPILER_MAJOR-NOTFOUND")
set(CMAKE_CXX_COMPILER_MINOR "CMAKE_CXX_COMPILER_MINOR-NOTFOUND")
set(CMAKE_CXX_COMPILER_PATCH "CMAKE_CXX_COMPILER_PATCH-NOTFOUND")

if(NOT WIN32)
if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
      OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)

  string(STRIP ${CMAKE_CXX_COMPILER_VERSION} CMAKE_CXX_COMPILER_VERSION)
  string(REGEX REPLACE "^([0-9]+).*$" "\\1"
         CMAKE_CXX_COMPILER_MAJOR ${CMAKE_CXX_COMPILER_VERSION})
  string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*$" "\\2"
         CMAKE_CXX_COMPILER_MINOR ${CMAKE_CXX_COMPILER_VERSION})
  string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" "\\3"
         CMAKE_CXX_COMPILER_PATCH ${CMAKE_CXX_COMPILER_VERSION})

  execute_process(COMMAND ${CMAKE_CXX_COMPILER} --version
    OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION_FULL)
#  if( ${CMAKE_CXX_COMPILER_VERSION_FULL} MATCHES "" )
#    set(
#  endif()

endif( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
endif(NOT WIN32)


# just print the results if requested
function(info_compiler)
  message(STATUS "CMAKE_FORCE_CXX_COMPILER  = '${CMAKE_FORCE_CXX_COMPILER}'")
  message(STATUS "CMAKE_CXX_COMPILER        = '${CMAKE_CXX_COMPILER}'")
  message(STATUS "CMAKE_CXX_COMPILER_ID     = '${CMAKE_CXX_COMPILER_ID}'")
  message(STATUS "CMAKE_CXX_COMPILER_INIT   = '${CMAKE_CXX_COMPILER_INIT}'")
  message(STATUS "CMAKE_GENERATOR_CXX       = '${CMAKE_GENERATOR_CXX}'")
  message(STATUS "CMAKE_GNULD_IMAGE_VERSION = '${CMAKE_GNULD_IMAGE_VERSION}'")
  message(STATUS "CMAKE_CXX_COMPILER_VERSION= '${CMAKE_CXX_COMPILER_VERSION}'")
  message(STATUS "CMAKE_CXX_COMPILER_MAJOR  = '${CMAKE_CXX_COMPILER_MAJOR}'")
  message(STATUS "CMAKE_CXX_COMPILER_MINOR  = '${CMAKE_CXX_COMPILER_MINOR}'")
  message(STATUS "CMAKE_CXX_COMPILER_PATCH  = '${CMAKE_CXX_COMPILER_PATCH}'")
  message(STATUS "CMAKE_SYSTEM_NAME         = '${CMAKE_SYSTEM_NAME}'")
  message(STATUS "CMAKE_SYSTEM_VERSION      = '${CMAKE_SYSTEM_VERSION}'")
  message(STATUS "CMAKE_SYSTEM_PROCESSOR    = '${CMAKE_SYSTEM_PROCESSOR}'")
endfunction(info_compiler)
