# -*- mode: cmake; -*-

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${Boost_INCLUDE_DIRS})
file(GLOB all_packettest_src *.cpp *.hpp)
set(packettest_src ${all_packettest_src})
add_executable(packettest ${packettest_src})

# Link the executable
target_link_libraries(packettest hexabus ${Boost_LIBRARIES} )

ADD_TEST(PacketTest ${CMAKE_CURRENT_BINARY_DIR}/packettest)
