# -*- mode: cmake; -*-

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${ALSA_INCLUDE_DIR}
)

file(GLOB all_rtmidi_src 
  *.cpp *.hpp *.h
)
set(rtmidi_src ${all_rtmidi_src})
ADD_LIBRARY(rtmidi ${rtmidi_src})

IF(HAS_LINUX)
    FIND_PACKAGE(Alsa)
    target_link_libraries(rtmidi ${ALSA_LIBRARIES})
    
    set(CMAKE_CXX_FLAGS "-D__LINUX_ALSA__ ")
ENDIF(HAS_LINUX)
