message("\t -- CANOpen service")

add_library(canopen_app OBJECT)

target_sources(canopen_app 
    PRIVATE 
    canopen_app.c
)

target_include_directories(canopen_app
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(canopen_app
    PRIVATE
    freertos_kernel
    canopen
)