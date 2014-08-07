# -*- mode: cmake; -*-

#set(hdrs 
#  common.hpp
#)
#install(FILES ${hdrs} DESTINATION include/hexabus)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})

file(GLOB all_hba_src *.cpp *.hpp)
set(hba_src ${all_hba_src})
ADD_LIBRARY(hba ${hba_src})

#target_link_libraries(hexabus ${Boost_IOSTREAMS_LIBRARY})

#if(BUILD_SHARED_LIBS)
#  install(TARGETS hb_assembler LIBRARY DESTINATION lib)
#else(BUILD_SHARED_LIBS)
#  install(TARGETS hb_assembler ARCHIVE DESTINATION lib)
#endif(BUILD_SHARED_LIBS)

