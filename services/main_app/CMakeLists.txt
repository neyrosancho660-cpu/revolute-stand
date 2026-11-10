message("\t -- Main Service")

add_library(main_app OBJECT)

target_sources(main_app 
    PRIVATE 
    main_app.c
)

target_include_directories(main_app
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(main_app
    PUBLIC
    cli
    pgv
    kinco
    freertos_kernel
)