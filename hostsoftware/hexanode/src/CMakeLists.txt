# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexanode_src "hexanode.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}/libhexanode
  ${Boost_INCLUDE_DIRS}
  ${RTMIDI_INCLUDE_DIR}
)
#link_directories(
#  #  ${SQLITE3_STATIC_LIBRARY_DIRS}
#  )

add_executable(hxbnode ${hexanode_src})
# Link the executables
target_link_libraries(hxbnode 
  hexanode
  pthread
  ${RTMIDI_LIBRARIES}
  ${Boost_LIBRARIES}
)

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hxbnode
  DESTINATION bin)

