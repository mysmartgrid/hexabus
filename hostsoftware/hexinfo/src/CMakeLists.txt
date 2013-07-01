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
  ${HBX_INCLUDE_DIR}
  ${HBC_INCLUDE_DIR}
)
#link_directories(
#  #  ${SQLITE3_STATIC_LIBRARY_DIRS}
#  )

add_executable(hexinfo ${hexinfo_src})
# Link the executables
target_link_libraries(hexinfo
  pthread
  ${HXB_LIBRARIES}
  ${HBC_LIBRARIES}
  ${Boost_LIBRARIES}
)

# add programs to the install target 
INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hexinfo
  DESTINATION bin)

