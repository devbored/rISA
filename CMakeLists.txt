cmake_minimum_required(VERSION 3.10)

project(risa)
set(RISA_DIR ${CMAKE_SOURCE_DIR}/risa)
set(GDBSTUB_DIR ${CMAKE_SOURCE_DIR}/external/gdbstub)
set(TEST_HANDLER_DIR ${CMAKE_SOURCE_DIR}/examples/test_handler)
set(TEST_ASM_PROG_DIR ${CMAKE_SOURCE_DIR}/examples/test_asm_program)
set(TEST_C_PROG_DIR ${CMAKE_SOURCE_DIR}/examples/test_c_program)

add_executable(risa 
    ${RISA_DIR}/main.c
    ${RISA_DIR}/risa.c
    ${RISA_DIR}/gdbstub_sys.c
    ${RISA_DIR}/socket.c
    ${GDBSTUB_DIR}/gdbstub.c
)
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
#target_compile_definitions(
#    risa
#    PUBLIC
#    -DDEBUG
#)
set_property(
    TARGET risa
    PROPERTY C_STANDARD 99
)

add_subdirectory(${CMAKE_SOURCE_DIR}/examples/test_handler)

# Example RISC-V asm and C program uses separate cross-compiler
# (Needs to run as separate CMake command due to this)
add_custom_target(TEST_PROG_ASM ALL
    COMMAND cmake ${TEST_ASM_PROG_DIR} -B${CMAKE_BINARY_DIR}/examples/test_asm_program -G ${CMAKE_GENERATOR}
    COMMAND cmake --build ${CMAKE_BINARY_DIR}/examples/test_asm_program
)
add_custom_target(TEST_PROG_C ALL
    COMMAND cmake ${TEST_C_PROG_DIR} -B${CMAKE_BINARY_DIR}/examples/test_c_program -G ${CMAKE_GENERATOR}
    COMMAND cmake --build ${CMAKE_BINARY_DIR}/examples/test_c_program
)