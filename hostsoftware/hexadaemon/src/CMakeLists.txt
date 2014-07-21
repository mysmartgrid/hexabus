# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexadaemon_src "hexadaemon.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${HXB_INCLUDE_DIR}
  "../../../shared"
)
if (UCI_FOUND)
  include_directories(${UCI_INCLUDE_DIR})
endif()

#link_directories(
#  #  ${SQLITE3_STATIC_LIBRARY_DIRS}
#  )

add_executable(hexadaemon ${hexadaemon_src} ${cpp_files})
# Link the executables
target_link_libraries(hexadaemon
  pthread
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)
if (UCI_FOUND)
  target_link_libraries(hexadaemon
    ${UCI_LIBRARY}
  )
endif()

# add programs to the install target 
INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hexadaemon
  DESTINATION bin)

