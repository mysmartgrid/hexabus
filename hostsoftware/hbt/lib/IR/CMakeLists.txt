# -*- mode: cmake; -*-

file(GLOB IR_src *.cpp *.hpp)
ADD_LIBRARY(lib_IR ${IR_src})
