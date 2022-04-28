cmake_minimum_required(VERSION 3.10)
project(risa_tests)

# GoogleTest
set(CMAKE_CXX_STANDARD 11)
find_package(GTest REQUIRED)

add_executable(risa_tests)
target_sources(risa_tests PRIVATE
    ${RISA_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_execution.cpp
)
target_include_directories(risa_tests PRIVATE
    ${RISA_DIR}
    ${TESTS_DIR}
    ${ARGPARSE_DIR}
    ${GDBSTUB_DIR}
)
if (MSVC)
    target_compile_options(risa_tests PRIVATE "/Wall")
    target_link_libraries(risa_tests PRIVATE wsock32 ws2_32)
else()
    target_compile_options(risa_tests PRIVATE "-Wall")
    target_compile_options(risa_tests PRIVATE "-pedantic")
    # Certain tests dont use all of the stub's functions - silence this
    target_compile_options(risa_tests PRIVATE "-Wno-unused-function")
endif()
target_link_libraries(risa_tests
    GTest::GTest
    GTest::Main
    ${CMAKE_DL_LIBS}
)