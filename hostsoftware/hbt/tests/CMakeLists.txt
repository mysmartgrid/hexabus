include_directories(${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

add_executable(as-dis as-dis.cpp)
add_executable(mc-opt mc-opt.cpp)
target_link_libraries(as-dis ${Boost_LIBRARIES} HBTMC)
target_link_libraries(mc-opt ${Boost_LIBRARIES} HBTMC)
add_test(as-dis ${CMAKE_CURRENT_BINARY_DIR}/as-dis)
add_test(mc-opt ${CMAKE_CURRENT_BINARY_DIR}/mc-opt)
add_test(hbt-lang-sema ${CMAKE_SOURCE_DIR}/tests/hbt-lang-sema/runner.sh)
