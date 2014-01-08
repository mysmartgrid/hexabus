# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexadaemon_src "hexadaemon.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${HXB_INCLUDE_DIR}
  ${UCI_INCLUDE_DIR}
  "../../../shared"
)
#link_directories(
#  #  ${SQLITE3_STATIC_LIBRARY_DIRS}
#  )

add_executable(hexadaemon ${hexadaemon_src} ${cpp_files})
# Link the executables
target_link_libraries(hexadaemon
  pthread
  ${UCI_LIBRARY}
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

# add programs to the install target 
INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hexadaemon
  DESTINATION bin)

