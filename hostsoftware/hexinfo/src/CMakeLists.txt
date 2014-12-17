# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexinfo_src "hexinfo.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${HXB_INCLUDE_DIR}
  ${HBC_INCLUDE_DIR}
)

add_executable(hexinfo ${hexinfo_src})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hexinfo DESTINATION bin)
target_link_libraries(hexinfo
  pthread
  ${HXB_LIBRARIES}
  ${HBT_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CLN_LIBRARIES}
)
