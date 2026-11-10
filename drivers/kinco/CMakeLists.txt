message("\t -- Kinco")

add_library(kinco STATIC)

target_sources(kinco 
    PRIVATE 
    kinco.c
)

target_include_directories(kinco
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kinco 
    PUBLIC
    log
    cli
    RTT
    TIMER
    bxCAN
    canopen
    freertos_kernel
)