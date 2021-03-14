cmake_minimum_required(VERSION 3.10)

project(risa)
set(RISA_DIR ${CMAKE_SOURCE_DIR}/risa)
set(TEST_HANDLER_DIR ${CMAKE_SOURCE_DIR}/examples/test_handler)
set(TEST_PROG_DIR ${CMAKE_SOURCE_DIR}/examples/test_program)

add_executable(risa 
    ${RISA_DIR}/risa.c
    ${RISA_DIR}/risa_core.c
)
target_link_libraries(
    risa
    ${CMAKE_DL_LIBS}
)
set_property(
    TARGET risa
    PROPERTY C_STANDARD 99
)

add_subdirectory(${CMAKE_SOURCE_DIR}/examples/test_handler)

# Example RISC-V asm program uses separate cross-compiler
# (Needs to run as separate CMake command due to this)
add_custom_target(TEST_PROG ALL
    COMMAND cmake ${TEST_PROG_DIR} -B${CMAKE_BINARY_DIR}/examples/test_program -G ${CMAKE_GENERATOR}
    COMMAND cmake --build ${CMAKE_BINARY_DIR}/examples/test_program
)