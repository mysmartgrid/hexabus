# -*- mode: cmake; -*-
# Common setting for Hexabus packages
#


#
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

#
if( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
   message(STATUS "Do not run cmake in the source directory")
   message(STATUS "create an extra binary directory")
   message(FATAL_ERROR "Exiting cmake here.")
endif( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

# Version settings
set(V_MAJOR 0)
set(V_MINOR 7)
set(V_PATCH 0)

# have the full monty in makefiles
set(CMAKE_VERBOSE_MAKEFILE true)

SET(ENABLE_LOGGING 1)

# enable unit testing
include(CTest)
enable_testing()

# add a path where some libraries might be stored
set(CMAKE_ADDITIONAL_PATH "$ENV{CMAKE_ADDITIONAL_PATH}" CACHE PATH "Path where many locally installed libraries can be found")

# Where are the additional libraries installed? Note: provide includes
# path here, subsequent checks will resolve everything else
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${CMAKE_ADDITIONAL_PATH}/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_ADDITIONAL_PATH}/lib)

#
include ( ProjectInfo )
include ( CompilerFlags )
include (UseCodeCoverage)

#
set(CMAKE_CXX_FLAGS "${CXXFLAGS} -std=c++11")

if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	# g++ fails to produce sensible warnings about missng field initializers in C++11 mode
	# because it warns *way* too much about code that is in no way incorrect
	# see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=36750#c11
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
endif()

#
set(Boost_USE_STATIC_LIBS ON)
SET(Boost_DETAILED_FAILURE_MSG true)
FIND_PACKAGE(Boost 1.54 REQUIRED COMPONENTS
  test_exec_monitor program_options
  filesystem regex system
  date_time
  thread
  )
