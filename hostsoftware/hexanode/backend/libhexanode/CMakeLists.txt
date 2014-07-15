# -*- mode: cmake; -*-
file(GLOB hdrs *.h *.hpp)
install(FILES ${hdrs} DESTINATION include/libhexanode)
file(GLOB midihdrs midi/*.h midi/*.hpp)
install(FILES ${midihdrs} DESTINATION include/libhexanode/midi)
file(GLOB callbackhdrs callbacks/*.h callbacks/*.hpp)
install(FILES ${callbackhdrs} DESTINATION include/libhexanode/callbacks)
file(GLOB hxbhdrs hexabus/*.h hexabus/*.hpp)
install(FILES ${hxbhdrs} DESTINATION include/libhexanode/hexabus)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/libhexanode/config.h)
install(FILES ${CMAKE_BINARY_DIR}/libhexanode/config.h DESTINATION
  include/libhexanode)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/include
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CPPNETLIB_INCLUDE_DIR}
  ${HXB_INCLUDE_DIR}
)

file(GLOB all_hexanode_src 
  *.cpp *.hpp
  midi/*.cpp midi/*.hpp
  callbacks/*.cpp callbacks/*.hpp
  hexabus/*.cpp hexabus/*.hpp
)
set(hexanode_src ${all_hexanode_src})
ADD_LIBRARY(hexanode ${hexanode_src})

#target_link_libraries(hexanode ${Boost_IOSTREAMS_LIBRARY})

if(BUILD_SHARED_LIBS)
  install(TARGETS hexanode LIBRARY DESTINATION lib)
else(BUILD_SHARED_LIBS)
  install(TARGETS hexanode ARCHIVE DESTINATION lib)
endif(BUILD_SHARED_LIBS)

