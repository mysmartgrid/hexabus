# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(hexadisplay_src "hexadisplay.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR}/libhexadisplay
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${HXB_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
)
add_executable(hexa-display
    ${hexadisplay_src}
    ${SAMPLE_MOC_SRCS}
    ${SAMPLE_RC_SRCS}
    ${SAMPLE_UI_HDRS}
  )
# Link the executables
target_link_libraries(hexa-display 
  hexadisplay
  klio
  pthread
  ${Boost_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${QT_LIBRARIES}
)

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hexa-display
  DESTINATION bin)

