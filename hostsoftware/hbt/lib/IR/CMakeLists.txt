# -*- mode: cmake; -*-

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

file(GLOB IR_src *.cpp *.hpp)
add_library(HBTIR ${IR_src})

if(BUILD_SHARED_LIBS)
	install(TARGETS HBTIR LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
	install(TARGETS HBTIR ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)
