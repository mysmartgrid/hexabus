# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
set(hbt_as_src "hbt-as.cpp")
set(hbc_src "hbc.cpp")
set(hbt_dis_src "hbt-dis.cpp")

add_executable(hbt-as ${hbt_as_src})
target_link_libraries(hbt-as
	lib_MC
	lib_Util
	${Boost_LIBRARIES}
)
install(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/hbt-as
	DESTINATION bin
)

add_executable(hbc ${hbc_src})
target_link_libraries(hbc
	lib_Lang
	lib_MC
	lib_IR
	lib_Util
	${Boost_LIBRARIES}
	${CLN_LIBRARIES}
)
install(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/hbc
	DESTINATION bin
)

add_executable(hbt-dis ${hbt_dis_src})
target_link_libraries(hbt-dis
	lib_MC
	lib_Util
	${Boost_LIBRARIES}
)
install(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/hbt-dis
	DESTINATION bin
)
