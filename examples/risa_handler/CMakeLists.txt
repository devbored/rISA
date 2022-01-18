add_library(risa_handler SHARED
    ${HANDLER_DIR}/risa_handler.c
)
target_include_directories(risa_handler PUBLIC
    ${RISA_DIR}
)
set_property(
    TARGET risa_handler
    PROPERTY C_STANDARD 99
)