# -*- mode: cmake; -*-


if(LIBKLIO_FOUND)
	file(GLOB hdrs *.h *.hpp logger/*.h logger/*.hpp)
	file(GLOB all_hexabus_src *.cpp *.hpp logger/*.cpp logger/*.hpp)
else()
	file(GLOB hdrs *.h *.hpp)
	file(GLOB all_hexabus_src *.cpp *.hpp)
endif()
install(FILES ${hdrs} DESTINATION include/libhexabus)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/libhexabus/config.h)
configure_file(private/paths.hpp.in ${CMAKE_BINARY_DIR}/libhexabus/private/paths.hpp)
install(FILES ${CMAKE_BINARY_DIR}/libhexabus/config.h DESTINATION
  include/libhexabus)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LIBKLIO_INCLUDE_DIR}
)
set(hexabus_src ${all_hexabus_src})
ADD_LIBRARY(hexabus ${hexabus_src})

#target_link_libraries(hexabus ${Boost_IOSTREAMS_LIBRARY})

if(BUILD_SHARED_LIBS)
  install(TARGETS hexabus LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexabus ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

