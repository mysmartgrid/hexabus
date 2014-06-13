file(GLOB hdrs *.h *.hpp)
file(GLOB all_rflib_src *.cpp)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LIBNL_INCLUDE_DIR}
  ${LIBGENL_INCLUDE_DIR}
  ${LIBNLROUTE_INCLUDE_DIR}
  ../include
)
ADD_LIBRARY(rflib ${all_rflib_src})
