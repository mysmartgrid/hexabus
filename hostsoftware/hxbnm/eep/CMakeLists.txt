file(GLOB hdrs *.h *.hpp)
file(GLOB all_eep_src *.cpp)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ../include
)
ADD_LIBRARY(eep ${all_eep_src})
