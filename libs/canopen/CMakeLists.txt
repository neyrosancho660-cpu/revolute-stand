message("\t -- canopen")

add_library(canopen OBJECT)

target_sources(canopen 
    PRIVATE 
    canopen.c
)

target_include_directories(canopen
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(canopen 
    PUBLIC
    CMSIS
    freertos_kernel
    bxCAN
)