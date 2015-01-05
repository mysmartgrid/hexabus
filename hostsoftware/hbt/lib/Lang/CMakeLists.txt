# -*- mode: cmake; -*-

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(${CLN_INCLUDE_DIR})

file(GLOB Lang_src *.cpp *.hpp)
add_library(HBTLang ${Lang_src})

set_source_files_properties(parser.cpp PROPERTIES COMPILE_FLAGS "-O2 -g0")

if(BUILD_SHARED_LIBS)
	install(TARGETS HBTLang LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
	install(TARGETS HBTLang ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)
