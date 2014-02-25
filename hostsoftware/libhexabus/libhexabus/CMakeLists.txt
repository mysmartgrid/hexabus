# -*- mode: cmake; -*-


file(GLOB hdrs *.h *.hpp)
file(GLOB all_hexabus_src *.cpp)

if(LIBKLIO_FOUND)
  file(GLOB logger_hdrs logger/*.h logger/*.hpp)
  file(GLOB logger_src  logger/*.cpp logger/*.hpp)
  include_directories( ${LIBKLIO_INCLUDE_DIR} )
endif()

install(FILES ${hdrs}        DESTINATION include/libhexabus)
install(FILES ${logger_hdrs} DESTINATION include/libhexabus/logger)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/libhexabus/config.h)
configure_file(private/paths.hpp.in ${CMAKE_BINARY_DIR}/libhexabus/private/paths.hpp)
install(FILES ${CMAKE_BINARY_DIR}/libhexabus/config.h DESTINATION
  include/libhexabus)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)
set(hexabus_src ${all_hexabus_src} ${logger_src})
ADD_LIBRARY(hexabus ${hexabus_src})

#target_link_libraries(hexabus ${Boost_IOSTREAMS_LIBRARY})

if(BUILD_SHARED_LIBS)
  install(TARGETS hexabus LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexabus ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

