# -*- mode: cmake; -*-

file(GLOB MC_src *.cpp *.hpp)
ADD_LIBRARY(lib_MC ${MC_src})
