# -*- mode: cmake; -*-

file(GLOB Lang_src *.cpp *.hpp)
ADD_LIBRARY(lib_Lang ${Lang_src})
