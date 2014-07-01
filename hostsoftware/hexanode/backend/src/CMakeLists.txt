# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(midi2hexabus_src "midi2hexabus.cpp")
set(hexabus_webbackend_src "hexabus_webbackend.cpp")
set(hexabus_msg_bridge_src "hexabus_msg_bridge.cpp")
set(simulate_energyflows_src "simulate_energyflows.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/include
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}/libhexanode
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${RTMIDI_INCLUDE_DIR}
  ${CPPNETLIB_INCLUDE_DIR}
  ${HXB_INCLUDE_DIR}
  ${LIBKLIO_INCLUDE_DIRS}
  ${SQLITE3_INCLUDE_DIR}
  ${CURL_INCLUDE_DIR}
)
#link_directories(
#  #  ${SQLITE3_STATIC_LIBRARY_DIRS}
#  )

add_executable(midi2hexabus ${midi2hexabus_src})
add_executable(hexabus_webbackend ${hexabus_webbackend_src})
add_executable(hexabus_msg_bridge ${hexabus_msg_bridge_src})
add_executable(simulate_energyflows ${simulate_energyflows_src})

if(LIBKLIO_ENABLE_ROCKSDB)
    set_source_files_properties(${hexabus_msg_bridge_src} COMPILE_FLAGS "-std=gnu++11")
endif()

# Link the executables
target_link_libraries(midi2hexabus 
  hexanode
  pthread
  rtmidi
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

target_link_libraries(hexabus_webbackend
  hexanode
  pthread
  ${CPPNETLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

target_link_libraries(hexabus_msg_bridge
  hexanode
  ${HXB_LIBRARIES}
  ${LIBKLIO_LIBRARY}
  ${Boost_LIBRARIES}
  ${Boost_SYSTEM_LIBRARY}
  ${JSON_LIBRARY}
  ${CURL_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${GNUTLS_LIBRARIES}
  ${OPENSSL_CRYPTO_LIBRARY}
  pthread
)
target_link_libraries(hexabus_msg_bridge ${ROCKSDB_LIBRARY})

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
  ${CMAKE_CURRENT_BINARY_DIR}/midi2hexabus
  ${CMAKE_CURRENT_BINARY_DIR}/hexabus_webbackend
  ${CMAKE_CURRENT_BINARY_DIR}/hexabus_msg_bridge
  ${CMAKE_CURRENT_BINARY_DIR}/simulate_energyflows
  DESTINATION bin)

