cmake_minimum_required(VERSION 3.10)

project(risa)
set(RISA_DIR ${CMAKE_SOURCE_DIR}/risa)
set(GDBSTUB_DIR ${CMAKE_SOURCE_DIR}/external/minigdbstub)
set(ARGPARSE_DIR ${CMAKE_SOURCE_DIR}/external/miniargparse)
set(TEST_HANDLER_DIR ${CMAKE_SOURCE_DIR}/examples/test_handler)
set(TEST_C_PROG_DIR ${CMAKE_SOURCE_DIR}/examples/test_program)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)
set(CROSS_GENERATOR)
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_RISCV_PROGRAM "Build example RISC-V program" OFF)
option(GDBLOG "Build with gdb packet log printing" OFF)
if(GDBLOG)
    add_definitions(-DGDBLOG)
endif(GDBLOG)

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
    ${ARGPARSE_DIR}
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

if(BUILD_TESTS)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif(BUILD_TESTS)
add_subdirectory(${CMAKE_SOURCE_DIR}/examples/test_handler)

# Example RISC-V C program uses separate cross-compiler
# (Needs to run as separate CMake command due to this)
if(BUILD_RISCV_PROGRAM)
    if(NOT DEFINED CROSS_GENERATOR)
        set(CROSS_GENERATOR "Unix Makefiles")
    endif()
    add_custom_target(TEST_PROG_C ALL
        COMMAND cmake ${TEST_C_PROG_DIR} -B${CMAKE_BINARY_DIR}/examples/test_program -G ${CROSS_GENERATOR}
        COMMAND cmake --build ${CMAKE_BINARY_DIR}/examples/test_program
    )
endif(BUILD_RISCV_PROGRAM)
