# -*- mode: cmake; -*-
set(6switch_src 6switch.cpp)
set(6switchd_src 6switchd.cpp)

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR} 
                    ${Boost_INCLUDE_DIRS})
add_executable(6switch ${6switch_src})
add_executable(6switchd ${6switchd_src})
#add_executable(foobar sourcetest.cpp)

# Link the executable 
target_link_libraries(6switch hexabus ${Boost_SYSTEM_LIBRARY})
target_link_libraries(6switchd hexabus ${Boost_SYSTEM_LIBRARY})

# add programs to the install target - do not install the hashbench benchmark program.
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/6switch
  ${CMAKE_CURRENT_BINARY_DIR}/6switchd
  DESTINATION bin)

