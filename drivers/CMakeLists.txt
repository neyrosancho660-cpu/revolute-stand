message("DRIVERS:")

add_library(drivers INTERFACE)

add_subdirectory(pgv)
add_subdirectory(kinco)
add_subdirectory(irayple_rs485)
add_subdirectory(irayple_canopen)
add_subdirectory(pepperl_fuchs_canopen)

target_link_libraries(drivers INTERFACE 
    pgv 
    kinco 
    irayple_rs485 
    irayple_canopen 
    pepperl_fuchs_canopen
)