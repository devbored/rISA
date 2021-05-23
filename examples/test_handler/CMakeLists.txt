add_library(test_handler SHARED 
    ${TEST_HANDLER_DIR}/test_handler.c
)
target_include_directories(test_handler PUBLIC
    ${RISA_DIR}
)
set_property(
    TARGET test_handler
    PROPERTY C_STANDARD 99
)