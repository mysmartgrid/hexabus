# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexinfo_src "hexinfo.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${Boost_INCLUDE_DIRS}
)
add_executable(hexinfo ${hexinfo_src})
# Link the executables
target_link_libraries(hexinfo 
  hexabus
  pthread
  ${Boost_SYSTEM_LIBRARY}
)

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hexinfo
  DESTINATION bin)

