# -*- mode: cmake; -*-

#set(hdrs 
#  common.hpp
#)
#install(FILES ${hdrs} DESTINATION include/hexabus)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
)
file(GLOB all_hbc_src *.cpp *.hpp hashlib2plue/*.h hashlib2plus/*.cpp)
set(hbc_src ${all_hbc_src})
ADD_LIBRARY(hbc ${hbc_src})

#target_link_libraries(hexabus ${Boost_IOSTREAMS_LIBRARY})

#if(BUILD_SHARED_LIBS)
#  install(TARGETS hb_assembler LIBRARY DESTINATION lib)
#else(BUILD_SHARED_LIBS)
#  install(TARGETS hb_assembler ARCHIVE DESTINATION lib)
#endif(BUILD_SHARED_LIBS)

