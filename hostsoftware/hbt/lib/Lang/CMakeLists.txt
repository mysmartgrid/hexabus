# -*- mode: cmake; -*-

file(GLOB Lang_src *.cpp *.hpp)
ADD_LIBRARY(lib_Lang ${Lang_src})

set_source_files_properties(parser.cpp PROPERTIES COMPILE_FLAGS "-O2 -g0")
