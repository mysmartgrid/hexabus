# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(common_lib "resolv.cpp")

set(hexaswitch_src "hexaswitch.cpp" ${common_lib})
set(hexalog_src "hexalog.cpp" ${common_lib})
set(hexaupload_src "hexaupload.cpp" ${common_lib})
set(hexapost_src "hexapost.cpp" ${common_lib})
set(hexapair_src "hexapair.cpp" ${common_lib})
set(hexatimed_src "hexatimed.cpp" ${common_lib})

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/libhexabus
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

if(LIBKLIO_FOUND)
  include_directories(
    ${LIBKLIO_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
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

add_executable(hexapair ${hexapair_src})
# Link the executables
target_link_libraries(hexapair
  hexabus
  pthread
  ${Boost_LIBRARIES}
)

add_executable(hexatimed ${hexatimed_src})
# Link the executables
target_link_libraries(hexatimed
  hexabus
  pthread
  ${Boost_LIBRARIES}
)

if(LIBKLIO_FOUND)
  add_executable(hexalog ${hexalog_src})
  target_link_libraries(hexalog
    hexabus
    ${LIBKLIO_LIBRARY}
    ${Boost_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
    ${JSON_LIBRARY}
    ${CURL_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${GNUTLS_LIBRARIES}
    pthread
    ${OPENSSL_CRYPTO_LIBRARY}
#    gcrypt
  )

endif(LIBKLIO_FOUND)

# add programs to the install target
INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hexaswitch
  ${CMAKE_CURRENT_BINARY_DIR}/hexaupload
  ${CMAKE_CURRENT_BINARY_DIR}/hexapair
  ${CMAKE_CURRENT_BINARY_DIR}/hexatimed
  DESTINATION bin)

if(LIBKLIO_FOUND)
  INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/hexalog
  DESTINATION bin)
endif(LIBKLIO_FOUND)
