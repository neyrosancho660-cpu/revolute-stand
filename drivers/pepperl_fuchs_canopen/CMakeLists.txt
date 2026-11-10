message("\t -- Pepperl+Fuchs CANOpen")

add_library(pepperl_fuchs_canopen STATIC)

target_sources(pepperl_fuchs_canopen 
    PRIVATE 
    pepperl_fuchs_canopen.c
)

target_include_directories(pepperl_fuchs_canopen
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pepperl_fuchs_canopen 
    PUBLIC
    canopen
    log
)