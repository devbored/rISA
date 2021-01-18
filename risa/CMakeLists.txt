cmake_minimum_required(VERSION 3.10)

project(risa)
add_executable(risa 
    risa.c
)
target_link_libraries(
    risa
    ${CMAKE_DL_LIBS}
)
set_property(
    TARGET risa
    PROPERTY C_STANDARD 99
)
add_library(example_handlers SHARED 
    example_handlers.c
)