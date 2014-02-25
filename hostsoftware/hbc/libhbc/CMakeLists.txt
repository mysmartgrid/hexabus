# -*- mode: cmake; -*-

file(GLOB hdrs *.h *.hpp)
install(FILES ${hdrs} DESTINATION include/libhbc)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

include_directories(
  ${CMAKE_SOURCE_DIR}
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${CMAKE_BINARY_DIR}
)
file(GLOB all_hbc_src *.cpp *.hpp hashlib2plue/*.h hashlib2plus/*.cpp)
set(hbc_src ${all_hbc_src})
ADD_LIBRARY(hbc ${hbc_src})

#target_link_libraries(hexabus ${Boost_IOSTREAMS_LIBRARY})

if(BUILD_SHARED_LIBS)
  install(TARGETS hbc LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hbc ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

