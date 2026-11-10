message("\t -- cli")

add_library(cli OBJECT)

target_sources(cli 
    PRIVATE 
    cli.c
)

target_include_directories(cli
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cli 
    PUBLIC
    RTT
)