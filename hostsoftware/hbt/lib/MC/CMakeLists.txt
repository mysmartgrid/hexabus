file(GLOB MC_src *.cpp *.hpp)
add_library(HBTMC ${MC_src})

set_source_files_properties(parser.cpp PROPERTIES COMPILE_FLAGS "-O2 -g0")

if(BUILD_SHARED_LIBS)
	install(TARGETS HBTMC LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
	install(TARGETS HBTMC ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)
