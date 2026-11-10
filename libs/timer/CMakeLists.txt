message("\t -- TIMER")

add_library(TIMER OBJECT)

target_sources(TIMER 
    PRIVATE 
    timer.c
)

target_include_directories(TIMER
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(TIMER PRIVATE CMSIS)