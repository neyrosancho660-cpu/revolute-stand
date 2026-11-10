message("\t -- PGV")

add_library(pgv STATIC)

target_sources(pgv 
    PRIVATE 
    pgv.c
)

target_include_directories(pgv
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pgv 
    PUBLIC
    cli
    TIMER
    irayple_rs485
    irayple_canopen
    pepperl_fuchs_canopen
    freertos_kernel
)