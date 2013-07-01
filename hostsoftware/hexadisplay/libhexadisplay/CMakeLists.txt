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
set(hexadisplay_forms simple.ui)
set(hexadisplay_resources default.qrc)
QT4_WRAP_UI(hexadisplay_hdr_forms ${hexadisplay_forms})
QT4_WRAP_CPP(hexadisplay_hdr_moc ${hexadisplay_hdr})
QT4_ADD_RESOURCES(hexadisplay_rcc_resources ${hexadisplay_resources})

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${HXB_INCLUDE_DIR}
)

ADD_LIBRARY(hexadisplay 
  ${hexadisplay_src} 
  ${hexadisplay_hdr_moc}
  ${hexadisplay_hdr_forms}
  ${hexadisplay_rcc_resources}
  )
target_link_libraries(hexadisplay 
  ${QT_LIBRARIES}
  ${HXB_LIBRARIES}
  )

if(BUILD_SHARED_LIBS)
  install(TARGETS hexadisplay LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexadisplay ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

