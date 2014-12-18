# -*- mode: cmake; -*-

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(${CLN_INCLUDE_DIR})

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
set(hbc_src "hbc.cpp")
set(hbt_as_src "hbt-as.cpp")
set(hbt_dis_src "hbt-dis.cpp")

add_executable(hbt-as ${hbt_as_src})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hbt-as DESTINATION bin)
target_link_libraries(hbt-as
	HBTMC HBTUtil
	${Boost_LIBRARIES}
)

add_executable(hbc ${hbc_src})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hbc DESTINATION bin)
target_link_libraries(hbc
	HBTLang HBTMC HBTIR HBTUtil
	${Boost_LIBRARIES} ${CLN_LIBRARIES}
)

add_executable(hbt-dis ${hbt_dis_src})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hbt-dis DESTINATION bin)
target_link_libraries(hbt-dis
	HBTMC HBTUtil
	${Boost_LIBRARIES}
)
