# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexaswitch_src "hexaswitch.cpp")
set(hexalog_src "hexalog.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${Boost_INCLUDE_DIRS}
)
add_executable(hexaswitch ${hexaswitch_src})
# Link the executables
target_link_libraries(hexaswitch 
  hexabus
  pthread
  ${Boost_SYSTEM_LIBRARY}
)

add_executable(hexalog ${hexalog_src})
target_link_libraries(hexalog
  hexabus
  pthread
  ${Boost_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  klio
  klio_sqlite3
)

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hexaswitch
  ${CMAKE_CURRENT_BINARY_DIR}/hexalog
  DESTINATION bin)

