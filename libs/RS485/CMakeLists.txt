message("\t -- rs485")

add_library(rs485 OBJECT)

target_sources(rs485 
    PRIVATE 
    rs485.c
)

target_include_directories(rs485
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rs485 
    PUBLIC
    CMSIS
    TIMER
)