file(GLOB Util_src *.cpp *.hpp)
add_library(HBTUtil ${Util_src})

if(BUILD_SHARED_LIBS)
	install(TARGETS HBTUtil LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
	install(TARGETS HBTUtil ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)
