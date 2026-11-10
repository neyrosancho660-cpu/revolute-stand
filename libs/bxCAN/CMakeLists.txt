message("\t -- bxCAN")

add_library(bxCAN OBJECT)

target_sources(bxCAN 
    PRIVATE 
    bxCAN.c
)

target_include_directories(bxCAN
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bxCAN 
    PUBLIC 
    CMSIS
    TIMER
)