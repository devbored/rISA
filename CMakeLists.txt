cmake_minimum_required(VERSION 3.10)

project(risa)
set(RISA_DIR ${CMAKE_SOURCE_DIR}/risa)
set(EXAMPLE_HANDLER_DIR ${CMAKE_SOURCE_DIR}/example_handlers)

add_executable(risa 
    ${RISA_DIR}/risa.c
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
    ${EXAMPLE_HANDLER_DIR}/example_handlers.c
)
target_include_directories(example_handlers PUBLIC
    ${RISA_DIR}
)
set_property(
    TARGET example_handlers
    PROPERTY C_STANDARD 99
)