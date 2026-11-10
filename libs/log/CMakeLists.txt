message("\t -- log")

add_library(log OBJECT)

target_sources(log 
    PRIVATE 
    log.c
)

target_include_directories(log
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(log 
    PUBLIC
    RTT
    freertos_kernel
)