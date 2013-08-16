# -*- mode: cmake; -*-


file(GLOB hdrs *.h *.hpp)
install(FILES ${hdrs} DESTINATION include/libhexabus)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/libhexabus
  ${LIBKLIO_INCLUDE_DIR}
)
file(GLOB all_hexalog_src *.cpp *.hpp)
set(hexalog_src ${all_hexalog_src})
ADD_LIBRARY(libhexalog ${hexalog_src})

if(BUILD_SHARED_LIBS)
  install(TARGETS libhexalog LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS libhexalog ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

