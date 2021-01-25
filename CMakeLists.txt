cmake_minimum_required(VERSION 3.10)

project(risa)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/risa)

add_executable(risa 
    ${SRC_DIR}/risa.c
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
    ${SRC_DIR}/example_handlers.c
)
set_property(
    TARGET example_handlers
    PROPERTY C_STANDARD 99
)