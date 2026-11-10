message("\t-- RTT")
add_library(RTT OBJECT)

target_sources(
    RTT
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/RTT/SEGGER_RTT.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RTT/SEGGER_RTT_printf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Syscalls/SEGGER_RTT_Syscalls_GCC.c
)

target_include_directories(
    RTT
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/RTT
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
)