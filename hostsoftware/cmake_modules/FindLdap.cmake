# -*- mode: cmake; -*-
# locates the ldap library
# This file defines:
# * LDAP_FOUND if ldap was found
# * LDAP_LIBRARY The lib to link to (currently only a static unix lib) 
# * LDAP_INCLUDE_DIR

if (NOT LDAP_FIND_QUIETLY)
  message(STATUS "FindGnuTls check")
endif (NOT LDAP_FIND_QUIETLY)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  include(FindPackageHandleStandardArgs)

  if (NOT WIN32)
    include(FindPkgConfig)
    if ( PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")

      pkg_check_modules (PC_LDAP ldap)

      set(LDAP_DEFINITIONS ${PC_LDAP_CFLAGS_OTHER})
      message(STATUS "==> '${PC_LDAP_CFLAGS_OTHER}'")
    else(PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")
      message(STATUS "==> N '${PC_LDAP_CFLAGS_OTHER}'")
    endif(PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")
  endif (NOT WIN32)

  #
  # set defaults
  set(_ldap_HOME "/usr/local")
  set(_ldap_INCLUDE_SEARCH_DIRS
    ${CMAKE_INCLUDE_PATH}
    /usr/local/include
    /usr/include
    )

  set(_ldap_LIBRARIES_SEARCH_DIRS
    ${CMAKE_LIBRARY_PATH}
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib
    )

  ##
  if( "${LDAP_HOME}" STREQUAL "")
    if("" MATCHES "$ENV{LDAP_HOME}")
      message(STATUS "LDAP_HOME env is not set, setting it to /usr/local")
      set (LDAP_HOME ${_ldap_HOME})
    else("" MATCHES "$ENV{LDAP_HOME}")
      set (LDAP_HOME "$ENV{LDAP_HOME}")
    endif("" MATCHES "$ENV{LDAP_HOME}")
  else( "${LDAP_HOME}" STREQUAL "")
    message(STATUS "LDAP_HOME is not empty: \"${LDAP_HOME}\"")
  endif( "${LDAP_HOME}" STREQUAL "")
  ##

  message(STATUS "Looking for ldap in ${LDAP_HOME}")

  if( NOT ${LDAP_HOME} STREQUAL "" )
    set(_ldap_INCLUDE_SEARCH_DIRS ${LDAP_HOME}/include ${_ldap_INCLUDE_SEARCH_DIRS})
    set(_ldap_LIBRARIES_SEARCH_DIRS ${LDAP_HOME}/lib ${_ldap_LIBRARIES_SEARCH_DIRS})
    set(_ldap_HOME ${LDAP_HOME})
  endif( NOT ${LDAP_HOME} STREQUAL "" )

  if( NOT $ENV{LDAP_INCLUDEDIR} STREQUAL "" )
    set(_ldap_INCLUDE_SEARCH_DIRS $ENV{LDAP_INCLUDEDIR} ${_ldap_INCLUDE_SEARCH_DIRS})
  endif( NOT $ENV{LDAP_INCLUDEDIR} STREQUAL "" )

  if( NOT $ENV{LDAP_LIBRARYDIR} STREQUAL "" )
    set(_ldap_LIBRARIES_SEARCH_DIRS $ENV{LDAP_LIBRARYDIR} ${_ldap_LIBRARIES_SEARCH_DIRS})
  endif( NOT $ENV{LDAP_LIBRARYDIR} STREQUAL "" )

  if( LDAP_HOME )
    set(_ldap_INCLUDE_SEARCH_DIRS ${LDAP_HOME}/include ${_ldap_INCLUDE_SEARCH_DIRS})
    set(_ldap_LIBRARIES_SEARCH_DIRS ${LDAP_HOME}/lib ${_ldap_LIBRARIES_SEARCH_DIRS})
    set(_ldap_HOME ${LDAP_HOME})
  endif( LDAP_HOME )

  # find the include files
  find_path(LDAP_INCLUDE_DIR ldap.h
    HINTS
    ${_ldap_INCLUDE_SEARCH_DIRS}
    ${PC_LDAP_INCLUDEDIR}
    ${PC_LDAP_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
    )

  # locate the library
  if(WIN32)
    set(LDAP_LIBRARY_NAMES ${LDAP_LIBRARY_NAMES} libldap.lib)
  else(WIN32)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      # On MacOS
      set(LDAP_LIBRARY_NAMES ${LDAP_LIBRARY_NAMES} libldap.dylib)
    else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(LDAP_LIBRARY_NAMES ${LDAP_LIBRARY_NAMES} libldap.a)
    endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  endif(WIN32)

  set(PC_LDAP_STATIC_LIBRARIES "sasl2")
  if( PC_LDAP_STATIC_LIBRARIES )
    foreach(lib ${PC_LDAP_STATIC_LIBRARIES})
      string(TOUPPER ${lib} _NAME_UPPER)

      find_library(LDAP_${_NAME_UPPER}_LIBRARY NAMES "lib${lib}.a"
	HINTS
	${_ldap_LIBRARIES_SEARCH_DIRS}
	${PC_LDAP_LIBDIR}
	${PC_LDAP_LIBRARY_DIRS}
	)
      #list(APPEND LDAP_LIBRARIES ${_dummy})
    endforeach()
    set(_LDAP_LIBRARIES "")
    foreach(lib ${PC_LDAP_STATIC_LIBRARIES} )
      string(TOUPPER ${lib} _NAME_UPPER)
      if( NOT ${LDAP_${_NAME_UPPER}_LIBRARY} STREQUAL "LDAP_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
	set(_LDAP_LIBRARIES ${_LDAP_LIBRARIES} ${LDAP_${_NAME_UPPER}_LIBRARY})
      else( NOT ${LDAP_${_NAME_UPPER}_LIBRARY} STREQUAL "LDAP_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
	set(_LDAP_LIBRARIES ${_LDAP_LIBRARIES} -l${lib})
      endif( NOT ${LDAP_${_NAME_UPPER}_LIBRARY} STREQUAL "LDAP_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
    endforeach()
    set(LDAP_LIBRARIES ${_LDAP_LIBRARIES} CACHE FILEPATH "")
  endif( PC_LDAP_STATIC_LIBRARIES )
  #else( PC_LDAP_STATIC_LIBRARIES )
  message("==> LDAP-search: ${_ldap_LIBRARIES_SEARCH_DIRS}")
    find_library(LDAP_LIBRARY NAMES ${LDAP_LIBRARY_NAMES}
      HINTS
      ${_ldap_LIBRARIES_SEARCH_DIRS}
      ${PC_LDAP_LIBDIR}
      ${PC_LDAP_LIBRARY_DIRS}
      )
  #endif( PC_LDAP_STATIC_LIBRARIES )

  message("==> LDAP_LIBRARY='${LDAP_LIBRARY}'")
  # On Linux
  find_library (LDAP_SHARED_LIBRARY
    NAMES libldap.so
    HINTS ${LDAP_HOME} ENV LDAP_HOME
    PATH_SUFFIXES lib
    )


#  if (LDAP_INCLUDE_DIR AND LDAP_LIBRARY)
#    set (LDAP_FOUND TRUE)
#    if (NOT LDAP_FIND_QUIETLY)
#      message (STATUS "Found ldap headers in ${LDAP_INCLUDE_DIR} and libraries ${LDAP_LIBRARY}")
#    endif (NOT LDAP_FIND_QUIETLY)
#  else (LDAP_INCLUDE_DIR AND LDAP_LIBRARY)
#    if (LDAP_FIND_REQUIRED)
#      message (FATAL_ERROR "ldap could not be found!")
#    endif (LDAP_FIND_REQUIRED)
#  endif (LDAP_INCLUDE_DIR AND LDAP_LIBRARY)

find_package_handle_standard_args(LDAP   DEFAULT_MSG LDAP_LIBRARY LDAP_INCLUDE_DIR)

else(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  set(LDAP_FOUND true)
  set(LDAP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/ldap ${CMAKE_BINARY_DIR}/ldap)
  set(LDAP_LIBRARY_DIR "")
  set(LDAP_LIBRARY ldap)
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})

