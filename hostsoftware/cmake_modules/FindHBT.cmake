# -*- mode: cmake; -*-
# - Find HBT
# Find the native HBT includes and libraries
#
#  HBT_INCLUDE_DIR - where to find .h
#  HBT_LIBRARIES   - List of libraries to link
#  HBT_FOUND       - True if HBT found.

if(HBT_INCLUDE_DIR)
	# Already in cache, be silent
	set(HBT_FIND_QUIETLY TRUE)
endif()

#
# set defaults
set(_hbt_HOME "/usr/local")
set(_hbt_INCLUDE_SEARCH_DIRS
	${CMAKE_INCLUDE_PATH}
	/usr/local/include
	/usr/include)

set(_hbt_LIBRARIES_SEARCH_DIRS
	${CMAKE_LIBRARY_PATH}
	/usr/local/lib
	/usr/lib)

##
set(_hbt_HOME "")
if("${HBT_HOME}" STREQUAL "")
	if("" MATCHES "$ENV{HBT_HOME}")
		message(STATUS "HBT_HOME env is not set, setting it to /usr/local")
		set(HBT_HOME ${_hbt_HOME})
	else()
		set(HBT_HOME "$ENV{HBT_HOME}")
	endif()
else()
	message(STATUS "HBT_HOME is not empty: \"${HBT_HOME}\"")
endif()

##
if(NOT ${HBT_HOME} STREQUAL "")
	set(_hbt_INCLUDE_SEARCH_DIRS ${HBT_HOME}/include ${_hbt_INCLUDE_SEARCH_DIRS})
	set(_hbt_LIBRARIES_SEARCH_DIRS ${HBT_HOME}/lib ${_hbt_LIBRARIES_SEARCH_DIRS})
	set(_hbt_HOME ${HBT_HOME})
endif()

if(NOT $ENV{HBT_INCLUDEDIR} STREQUAL "")
	set(_hbt_INCLUDE_SEARCH_DIRS $ENV{HBT_INCLUDEDIR} ${_hbt_INCLUDE_SEARCH_DIRS})
endif()

if(NOT $ENV{HBT_LIBRARYDIR} STREQUAL "")
	set(_hbt_LIBRARIES_SEARCH_DIRS $ENV{HBT_LIBRARYDIR} ${_hbt_LIBRARIES_SEARCH_DIRS})
endif()

if(HBT_HOME)
	set(_hbt_INCLUDE_SEARCH_DIRS ${HBT_HOME}/include ${_hbt_INCLUDE_SEARCH_DIRS})
	set(_hbt_LIBRARIES_SEARCH_DIRS ${HBT_HOME}/lib ${_hbt_LIBRARIES_SEARCH_DIRS})
	set(_hbt_HOME ${HBT_HOME})
endif()


find_path(HBT_INCLUDE_DIR hbt/Lang/ast.hpp
	HINTS
	${_hbt_INCLUDE_SEARCH_DIRS}
	${PC_HBT_INCLUDEDIR}
	${PC_HBT_INCLUDE_DIRS}
	${CMAKE_INCLUDE_PATH})

# locate the library
if(WIN32)
	set(HBT_LIB_SUFFIX .lib)
else()
	set(HBT_LIB_SUFFIX .a)
endif()
set(_allfound TRUE)
foreach(lib IR Lang MC Util)
	find_library(HBT_${lib}_LIB NAMES libHBT${lib}${HBT_LIB_SUFFIX}
		HINTS
		${_hbt_LIBRARIES_SEARCH_DIRS}
		${PC_HBT_LIBDIR}
		${PC_HBT_LIBRARY_DIRS})
	if(NOT HBT_${lib}_LIB)
		set(_allfound FALSE)
	endif()
	set(HBT_LIBRARIES ${HBT_LIBRARIES} ${HBT_${lib}_LIB})
endforeach()
if(NOT _allfound)
	set(HBT_LIBRARIES "HBT-NOTFOUND")
endif()

# handle the QUIETLY and REQUIRED arguments and set HBT_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HBT DEFAULT_MSG HBT_LIBRARIES HBT_INCLUDE_DIR)

# if the include and the program are found then we have it
if(HBT_INCLUDE_DIR AND HBT_LIBRARIES)
	set(HBT_FOUND "YES")
endif()

mark_as_advanced (
	HBT_FOUND
	HBT_LIBRARIES
	HBT_INCLUDE_DIR)
