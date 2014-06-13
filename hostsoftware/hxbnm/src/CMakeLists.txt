file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")

set(hxbnm_src "hxbnm.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/libhexabus
  ${Boost_INCLUDE_DIRS}
  ${LIBNL_INCLUDE_DIR}
  ${LIBGENL_INCLUDE_DIR}
  ${LIBNLROUTE_INCLUDE_DIR}
  ../include
)

add_executable(hxbnm ${hxbnm_src})
target_link_libraries(hxbnm
  rflib
  eep
  rt
  ${Boost_LIBRARIES}
  ${LIBGENL_LIBRARIES}
  ${LIBNLROUTE_LIBRARIES}
  ${LIBNL_LIBRARIES}
)

INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hxbnm
  DESTINATION bin)
