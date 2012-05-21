# -*- mode: cmake; -*-

#set(hdrs 
#  common.hpp
#)
#install(FILES ${hdrs} DESTINATION include/hexadisplay)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

#add_subdirectory(messages)
#ADD_DEPENDENCIES(hexadisplay hexadisplay_sqlite3)


file(GLOB all_hexadisplay_src *.cpp *.hpp)
set(hexadisplay_src ${all_hexadisplay_src})
set(hexadisplay_hdr main_window.hpp)
QT4_WRAP_CPP(hexadisplay_hdr_moc ${hexadisplay_hdr})

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_LIBRARY(hexadisplay ${hexadisplay_src} ${hexadisplay_hdr_moc})
target_link_libraries(hexadisplay ${QT_LIBRARIES})

if(BUILD_SHARED_LIBS)
  install(TARGETS hexadisplay LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexadisplay ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

