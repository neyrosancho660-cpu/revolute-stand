message("\t -- CLI App Service")

add_library(cli_app OBJECT)

target_sources(cli_app
    PRIVATE
    cli_app.c
)

target_include_directories(cli_app
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cli_app
    PUBLIC
    freertos_kernel
    RTT
    cli
    kinco
    pgv
    main_app
)