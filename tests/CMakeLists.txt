cmake_minimum_required(VERSION 3.10)

project(risa_tests)

# External dependencies
find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)

file(GLOB SRCS *.cpp)
add_executable(risa_tests ${SRCS} ${RISA_SRCS})
target_include_directories(risa_tests PRIVATE
    ${RISA_DIR}
    ${TESTS_DIR}
    ${ARGPARSE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/minigdbstub
)
target_compile_options(risa_tests PRIVATE "--std=c++11")
if (MSVC)
    target_compile_options(risa_tests PRIVATE "/W4")
else()
    target_compile_options(risa_tests PRIVATE -Wall)
    target_compile_options(risa_tests PRIVATE -pedantic)
    target_compile_options(risa_tests PRIVATE -Wno-unused-function)
endif()
target_link_libraries(risa_tests
    GTest::gtest
    GTest::gtest_main
)