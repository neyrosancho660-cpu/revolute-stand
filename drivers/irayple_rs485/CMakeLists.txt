message("\t -- Irayple RS485")

add_library(irayple_rs485 STATIC)

target_sources(irayple_rs485 
    PRIVATE 
    irayple_rs485.c
)

target_include_directories(irayple_rs485
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(irayple_rs485 
    PUBLIC
    rs485
    RTT
    log
)