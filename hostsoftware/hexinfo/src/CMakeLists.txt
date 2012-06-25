# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
file(GLOB all_hexinfo_src *.cpp *.hpp)
set(hexinfo_src ${all_hexinfo_src})
configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

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
	${Boost_LIBRARIES}
)

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hexinfo
  DESTINATION bin)

