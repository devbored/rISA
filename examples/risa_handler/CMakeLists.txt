add_library(risa_handler SHARED
    ${HANDLER_DIR}/risa_handler.c
)
target_include_directories(risa_handler PUBLIC
    ${RISA_DIR}
)
set_target_properties(risa_handler
    PROPERTIES
        C_STANDARD 99
)
if (WIN32 OR MINGW)
    set_target_properties(risa_handler
        PROPERTIES
            PREFIX ""
            SUFFIX ".dll"
    )
endif()