message("\t-- CMSIS")
add_library(CMSIS INTERFACE)

target_include_directories(
    CMSIS
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Device/ST/STM32F1xx/Include
)

target_compile_definitions(
    CMSIS
    INTERFACE
    -DSTM32F103xE
)