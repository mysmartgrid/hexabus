# -*- mode: cmake; -*-
set(6switch_src 6switch.cpp)

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR} 
                    ${Boost_INCLUDE_DIRS})
add_executable(6switch ${6switch_src})
#add_executable(foobar sourcetest.cpp)

# Link the executable 
target_link_libraries(6switch hexabus )

# add programs to the install target - do not install the hashbench benchmark program.
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/6switch
  DESTINATION bin)

