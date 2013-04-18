# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(common_lib "resolv.cpp")

set(hexaswitch_src "hexaswitch.cpp" ${common_lib})
set(hexalog_src "hexalog.cpp" ${common_lib})
set(hexaupload_src "hexaupload.cpp" ${common_lib})

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/libhexabus
  ${Boost_INCLUDE_DIRS}
)
if(LIBKLIO_FOUND)
  include_directories(
  ${SQLITE3_STATIC_LIBRARY_DIRS}
  )
  link_directories(
  ${SQLITE3_STATIC_LIBRARY_DIRS}
  )
endif(LIBKLIO_FOUND)

add_executable(hexaswitch ${hexaswitch_src})
# Link the executables
target_link_libraries(hexaswitch
  hexabus
  pthread
  ${Boost_LIBRARIES}
)

add_executable(hexaupload ${hexaupload_src})
# Link the executables
target_link_libraries(hexaupload
  hexabus
  pthread
  ${Boost_LIBRARIES}
)

if(LIBKLIO_FOUND)
  add_executable(hexalog ${hexalog_src})
  target_link_libraries(hexalog
    hexabus
    pthread
    ${Boost_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
    curl
    json
    crypto
    ${LIBKLIO_LIBRARY}
    ${SQLITE3_LIBRARIES}
  )
endif(LIBKLIO_FOUND)

# add programs to the install target
INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hexaswitch
  ${CMAKE_CURRENT_BINARY_DIR}/hexaupload
  DESTINATION bin)

if(LIBKLIO_FOUND)
  INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hexalog
  DESTINATION bin)
endif(LIBKLIO_FOUND)
