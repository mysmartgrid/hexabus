# -*- mode: cmake; -*-

set(hexaswitch_src "hexaswitch.cpp")
set(hexalog_src "hexalog.cpp")
set(hexaupload_src "hexaupload.cpp")
set(hexapair_src "hexapair.cpp")
set(hexatimed_src "hexatimed.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/libhexabus
  ${CMAKE_SOURCE_DIR}/../../shared
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

if(LIBKLIO_FOUND)
  include_directories(
    ${LIBMYSMARTGRID_INCLUDE_DIR}
    ${LIBKLIO_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR}
  )
  link_directories(
  ${SQLITE3_STATIC_LIBRARY_DIRS}
  ${LIBMYSMARTGRID_LIBRARY}
  )
  if(LIBKLIO_ENABLE_ROCKSDB)
    set_source_files_properties(${hexalog_src} COMPILE_FLAGS "-std=gnu++11")
  endif()
endif(LIBKLIO_FOUND)

add_executable(hexaswitch ${hexaswitch_src})
# Link the executables
target_link_libraries(hexaswitch
  hexabus
  pthread
	rt
  ${Boost_LIBRARIES}
)

add_executable(hexaupload ${hexaupload_src})
# Link the executables
target_link_libraries(hexaupload
  hexabus
  pthread
	rt
  ${Boost_LIBRARIES}
)

add_executable(hexapair ${hexapair_src})
# Link the executables
target_link_libraries(hexapair
  hexabus
  pthread
	rt
  ${Boost_LIBRARIES}
)

add_executable(hexatimed ${hexatimed_src})
# Link the executables
target_link_libraries(hexatimed
  hexabus
  pthread
	rt
  ${Boost_LIBRARIES}
)

add_executable(hexasm "hexasm.cpp")
target_link_libraries(hexasm hexabus pthread ${Boost_LIBRARIES})

add_executable(hexalisten "hexalisten.cpp")
target_link_libraries(hexalisten hexabus pthread ${Boost_LIBRARIES})

add_executable(hexasend "hexasend.cpp")
target_link_libraries(hexasend hexabus pthread ${Boost_LIBRARIES})

if(NOT WITHOUT_HEXAJUICE)
  add_executable(hexajuice "hexajuice.cpp")
  target_link_libraries(hexajuice hexabus pthread ${Boost_LIBRARIES})
endif()

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
    ${ROCKSDB_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
		rt
#    gcrypt
  )

endif(LIBKLIO_FOUND)

set(HXB_EXECUTABLES
  ${CMAKE_CURRENT_BINARY_DIR}/hexaswitch
  ${CMAKE_CURRENT_BINARY_DIR}/hexaupload
  ${CMAKE_CURRENT_BINARY_DIR}/hexapair
  ${CMAKE_CURRENT_BINARY_DIR}/hexatimed
  ${CMAKE_CURRENT_BINARY_DIR}/hexasm
  ${CMAKE_CURRENT_BINARY_DIR}/hexalisten
  ${CMAKE_CURRENT_BINARY_DIR}/hexasend
  )

if(NOT WITHOUT_HEXAJUICE)
  set(HXB_EXECUTABLES
    ${HXB_EXECUTABLES}
    ${CMAKE_CURRENT_BINARY_DIR}/hexajuice
  )
endif()

if(LIBKLIO_FOUND)
  set(HXB_EXECUTABLES
  ${HXB_EXECUTABLES}
  ${CMAKE_CURRENT_BINARY_DIR}/hexalog
  )
endif(LIBKLIO_FOUND)

# add programs to the install target
INSTALL(PROGRAMS
  ${HXB_EXECUTABLES}
  DESTINATION bin)
