cmake_minimum_required(VERSION 3.10)

project(risa)
set(RISA_DIR ${CMAKE_SOURCE_DIR}/risa)
set(GDBSTUB_DIR ${CMAKE_SOURCE_DIR}/external/minigdbstub)
set(TEST_HANDLER_DIR ${CMAKE_SOURCE_DIR}/examples/test_handler)
set(TEST_C_PROG_DIR ${CMAKE_SOURCE_DIR}/examples/test_program)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

set(RISA_SRCS
    ${RISA_DIR}/risa.c
    ${RISA_DIR}/gdbserver.c
    ${RISA_DIR}/socket.c
)

add_executable(risa ${RISA_DIR}/main.c ${RISA_SRCS})
target_link_libraries(
    risa
    ${CMAKE_DL_LIBS}
)
target_include_directories(
    risa
    PUBLIC
    ${GDBSTUB_DIR}
    ${RISA_DIR}
)
target_compile_options(
    risa
    PUBLIC
    -Wall
    -pedantic
)
set_property(
    TARGET risa
    PROPERTY C_STANDARD 99
)

add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
add_subdirectory(${CMAKE_SOURCE_DIR}/examples/test_handler)

# Example RISC-V C program uses separate cross-compiler
# (Needs to run as separate CMake command due to this)
add_custom_target(TEST_PROG_C ALL
    COMMAND cmake ${TEST_C_PROG_DIR} -B${CMAKE_BINARY_DIR}/examples/test_program -G ${CMAKE_GENERATOR}
    COMMAND cmake --build ${CMAKE_BINARY_DIR}/examples/test_program
)
