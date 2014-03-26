# -*- mode: cmake; -*-

include ( CheckCompiler )
info_compiler()

if (${CMAKE_BUILD_TYPE} MATCHES "Release")
  add_definitions("-DNDEBUG")
endif()

if (NOT ENABLE_BACKTRACE_ON_PARSE_ERROR)
  add_definitions("-DNO_BACKTRACE_ON_PARSE_ERROR")
endif()

# Detect the system we're compiling on
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(HAS_MACOS 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(HAS_MACOS 0)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(HAS_LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(HAS_LINUX 0)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


###
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_INIT} $ENV{LDFLAGS}
  CACHE STRING "Flags used by the linker during the creation of shared libraries")
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_INIT} $ENV{LDFLAGS}
  CACHE STRING "Flags used by the linker during the creation of modules")

set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -W")
set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wall")
set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wextra")

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-unknown-warning-option")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-parentheses")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-constant-logical-operand")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-format-zero-length")
endif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-attributes")

set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-non-virtual-dtor")
set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-system-headers")

set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-unused-parameter")
set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-unused-variable")
set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-unknown-pragmas")
set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-type-limits")
set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-uninitialized")

if( NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips" )
# OR  ${CMAKE_CXX_COMPILER_MAJOR}>=4 AND 
#      ${CMAKE_CXX_COMPILER_MINOR}4))  # gcc version <= 4.3.3
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-unused-but-set-variable") #not supported by openwrt build toolchain
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-delete-non-virtual-dtor") #not supported by openwrt build toolchain
endif()

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-deprecated-writable-strings")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-unneeded-internal-declaration")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-overloaded-virtual")
endif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-write-strings")
  set (FLAGS_WARNINGS "${FLAGS_WARNINGS} -Wno-format")
endif (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")

# to avoid warnings when using gcc 4.5
add_definitions ("-fno-strict-aliasing")

if(NOT WIN32)
  if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    include (CheckCXXSourceCompiles)

    set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -fpic -fPIC ${FLAGS_WARNING}")

    set (CMAKE_REQUIRED_FLAGS "-Wno-ignored-qualifiers")
    set (__CXX_FLAG_CHECK_SOURCE "int main () { return 0; }")
    CHECK_CXX_SOURCE_COMPILES ("${__CXX_FLAG_CHECK_SOURCE}" W_NO_IGNORED_QUALIFIERS)
    set (CMAKE_REQUIRED_FLAGS)
    if (W_NO_IGNORED_QUALIFIERS)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-qualifiers")
    endif (W_NO_IGNORED_QUALIFIERS)

    ## Releases are made with the release build. Optimize code.
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fstack-protector-all") #not supported by openwrt build toolchain
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-missing-field-initializers")
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${FLAGS_WARNINGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror"
#      CACHE STRING "Flags for Release build"
      )
    message("===>REL-Flags: '${CMAKE_CXX_FLAGS_RELEASE}'")
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT
      ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT} "-rdynamic")

    ## debug flags
    set(CMAKE_CXX_FLAGS_DEBUG
      "-O0 -g -ggdb -fno-omit-frame-pointer ${FLAGS_WARNINGS}"
      )
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT ${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT} "-rdynamic")

    ## gprof and gcov support
    set(CMAKE_CXX_FLAGS_PROFILE "-O0 -pg -g -ggdb -W -Wreturn-type -Wno-shadow")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -fprofile-arcs")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -ftest-coverage")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} ${FLAGS_WARNINGS}"
      CACHE STRING "Flags for Profile build"
      )

    ## Experimental flags
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "-O0 -pg -g -ggdb -Werror -W -Wshadow")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -fprofile-generate ")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} ${FLAGS_WARNINGS}"
      CACHE STRING "Flags for Experimental build"
      )

  endif ()
  if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CXXFLAGS} -fPIC ${FLAGS_WARNINGS}")
  endif()

  # TODO: we need to check the compiler here, gcc does not know about those flags, is this The Right Thing To Do (TM)?
  if (${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
    set(CMAKE_CXX_FLAGS "${CXXFLAGS} -wd191 -wd1170 -wd1292 -wd2196 -fPIC -fpic")
    if (${CMAKE_CXX_COMPILER_MAJOR} GREATER 9)
      message(WARNING "adding __aligned__=ignored to the list of definitions")
      add_definitions("-D__aligned__=ignored")
    endif (${CMAKE_CXX_COMPILER_MAJOR} GREATER 9)
  endif (${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")

else(NOT WIN32)
#  set(CMAKE_CXX_FLAGS " /DWIN32 /D_WINDOWS /W3 /Zm1000 /EHsc /GR ")
#  set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /D NDEBUG")

endif(NOT WIN32)
