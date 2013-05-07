# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexanode_src "hexanode.cpp")
set(demo_generator_src "demo_generator.cpp")
set(push_values_src "push_values.cpp")
set(simulate_energyflows_src "simulate_energyflows.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/include
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
add_executable(demo_generator ${demo_generator_src})
add_executable(push_values ${push_values_src})
add_executable(simulate_energyflows ${simulate_energyflows_src})

# Link the executables
target_link_libraries(hxbnode 
  hexanode
  pthread
  rtmidi
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

target_link_libraries(demo_generator
  hexanode
  ${CPPNETLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${Boost_LIBRARIES}
)

target_link_libraries(push_values
  hexanode
  pthread
  ${CPPNETLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

target_link_libraries(simulate_energyflows
  hexanode
  pthread
  ${CPPNETLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hxbnode
  ${CMAKE_CURRENT_BINARY_DIR}/push_values
  ${CMAKE_CURRENT_BINARY_DIR}/simulate_energyflows
  DESTINATION bin)

