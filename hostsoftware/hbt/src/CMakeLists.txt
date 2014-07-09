# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
set(hbt_as_src "hbt-as.cpp")

add_executable(hbt-as ${hbt_as_src})
# Link the executables
target_link_libraries(hbt-as
	lib_IR
	lib_MC
)

# add programs to the install target
INSTALL(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/hbt-as
	DESTINATION bin
)
