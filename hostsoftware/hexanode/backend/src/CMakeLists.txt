# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(midi2hexabus_src "midi2hexabus.cpp")
set(hexabus_msg_bridge_src "hexabus_msg_bridge.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}/libhexanode
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(
  ${RTMIDI_INCLUDE_DIR}
  ${HXB_INCLUDE_DIR}
  ${LIBMYSMARTGRID_INCLUDE_DIR}
  ${LIBMYSMARTGRID_INCLUDE_DIRS}
  ${LIBKLIO_INCLUDE_DIRS}
  ${SQLITE3_INCLUDE_DIR}
  ${CURL_INCLUDE_DIR}
)

add_executable(midi2hexabus ${midi2hexabus_src})
add_executable(hexabus_msg_bridge ${hexabus_msg_bridge_src})

if(LIBKLIO_ENABLE_ROCKSDB)
    set_source_files_properties(${hexabus_msg_bridge_src} COMPILE_FLAGS "-std=gnu++11")
endif()

target_link_libraries(midi2hexabus 
  hexanode
  pthread
  rtmidi
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
  dl rt
)

target_link_libraries(hexabus_msg_bridge
  hexanode
  ${HXB_LIBRARIES}
  ${LIBKLIO_LIBRARY}
  ${LIBMYSMARTGRID_LIBRARY}
  ${Boost_LIBRARIES}
  ${Boost_SYSTEM_LIBRARY}
#  ${JSON_LIBRARY}
#  ${CURL_LIBRARIES}
#  ${SQLITE3_LIBRARIES}
#  ${GNUTLS_LIBRARIES}
#  ${OPENSSL_CRYPTO_LIBRARY}
  pthread
)
target_link_libraries(hexabus_msg_bridge ${ROCKSDB_LIBRARY})

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/midi2hexabus
  ${CMAKE_CURRENT_BINARY_DIR}/hexabus_msg_bridge
  DESTINATION bin)

