message("\t -- Irayple CANOpen")

add_library(irayple_canopen OBJECT)

target_sources(irayple_canopen 
    PRIVATE 
    irayple_canopen.c
)

target_include_directories(irayple_canopen
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(irayple_canopen 
    PUBLIC
    canopen
    log
)