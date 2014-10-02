include_directories(${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

add_executable(as-dis as-dis.cpp)
target_link_libraries(as-dis ${Boost_LIBRARIES} lib_IR lib_MC)
add_test(as-dis ${CMAKE_CURRENT_BINARY_DIR}/as-dis)
add_test(hbt-lang-sema ${CMAKE_SOURCE_DIR}/tests/hbt-lang-sema/runner.sh)
