file(GLOB IR_hdrs hbt/IR/*.hpp)
file(GLOB Lang_hdrs hbt/Lang/*.hpp)
file(GLOB MC_hdrs hbt/MC/*.hpp)
file(GLOB Util_hdrs hbt/Util/*.hpp)

install(FILES ${IR_hdrs} DESTINATION include/hbt/IR)
install(FILES ${Lang_hdrs} DESTINATION include/hbt/Lang)
install(FILES ${MC_hdrs} DESTINATION include/hbt/MC)
install(FILES ${Util_hdrs} DESTINATION include/hbt/Util)
