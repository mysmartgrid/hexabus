# -*- mode: cmake; -*-

#set(hdrs 
#  common.hpp
#)
#install(FILES ${hdrs} DESTINATION include/hexadisplay)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

#add_subdirectory(messages)
#ADD_DEPENDENCIES(hexadisplay hexadisplay_sqlite3)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
)
file(GLOB all_hexadisplay_src *.cpp *.hpp)
set(hexadisplay_src ${all_hexadisplay_src})
ADD_LIBRARY(hexadisplay ${hexadisplay_src})

#target_link_libraries(hexadisplay ${Boost_IOSTREAMS_LIBRARY})

if(BUILD_SHARED_LIBS)
  install(TARGETS hexadisplay LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexadisplay ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

