# -*- mode: cmake; -*-

#set(hdrs 
#  common.hpp
#)
#install(FILES ${hdrs} DESTINATION include/hexabus)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

#add_subdirectory(messages)
#ADD_DEPENDENCIES(hexabus hexabus_sqlite3)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
)
file(GLOB all_hexabus_src *.cpp *.hpp)
set(hexabus_src ${all_hexabus_src})
ADD_LIBRARY(hexabus ${hexabus_src})

#target_link_libraries(hexabus ${Boost_IOSTREAMS_LIBRARY})

if(BUILD_SHARED_LIBS)
  install(TARGETS hexabus LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexabus ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

