# -*- mode: cmake; -*-


file(GLOB hdrs *.h *.hpp)
file(GLOB all_hexabus_src *.cpp)

if(LIBKLIO_FOUND)
  file(GLOB logger_hdrs logger/*.h logger/*.hpp)
  file(GLOB logger_src  logger/*.cpp logger/*.hpp)
  include_directories( ${LIBKLIO_INCLUDE_DIRS} )
endif()

file(GLOB sm_hdrs sm/*.h sm/*.hpp)
file(GLOB sm_src  sm/*.cpp sm/*.hpp)

#meh
set_source_files_properties(sm/machine.cpp PROPERTIES OBJECT_DEPENDS ${HXB_SHARED}/sm_machine.c)

install(FILES ${hdrs}        DESTINATION include/libhexabus)
install(FILES ${logger_hdrs} DESTINATION include/libhexabus/logger)
install(FILES ${sm_hdrs}     DESTINATION include/libhexabus/sm)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/libhexabus/config.h)
configure_file(private/paths.hpp.in ${CMAKE_BINARY_DIR}/libhexabus/private/paths.hpp)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)
set(hexabus_src ${all_hexabus_src} ${logger_src} ${sm_src})
ADD_LIBRARY(hexabus ${hexabus_src})

#target_link_libraries(hexabus ${Boost_IOSTREAMS_LIBRARY})

if(BUILD_SHARED_LIBS)
  install(TARGETS hexabus LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexabus ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

