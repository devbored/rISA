cmake_minimum_required(VERSION 3.10)

set(CMAKE_SYSTEM_NAME      Generic)
set(CROSS_COMPILE          riscv64-unknown-elf-)
set(CMAKE_AS               "${CROSS_COMPILE}as")
set(CMAKE_OBJCOPY          "${CROSS_COMPILE}objcopy")
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

project(examples)

add_custom_command(OUTPUT "basic1.o"
    COMMAND ${CMAKE_AS} ${CMAKE_SOURCE_DIR}/basic1.s -o basic1.o
    COMMAND ${CMAKE_OBJCOPY} -O binary "${CMAKE_BINARY_DIR}/basic1.o" "${CMAKE_BINARY_DIR}/basic1.hex"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "basic1.s --> basic1.o --> basic1.hex"
)

add_custom_target(basic1 ALL
    DEPENDS "basic1.o"
)
