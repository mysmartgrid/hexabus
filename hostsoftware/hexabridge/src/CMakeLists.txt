# -*- mode: cmake; -*-
set(hexabridge_src main.cpp hexabridge.cpp)
link_directories(${SQLITE3_LIBRARY_DIRS})

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${LIBMYSMARTGRID_INCLUDE_DIRS}
                    ${HXB_INCLUDE_DIR}
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

add_executable(hexabridge ${hexabridge_src})
target_link_libraries(hexabridge
                      ${Boost_LIBRARIES}
                      ${LIBMYSMARTGRID_LIBRARY}
                      ${LIBMYSMARTGRID_LIBRARIES}
                      ${HXB_LIBRARIES}
)

# sensor programs to the install target
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hexabridge
  DESTINATION bin)
