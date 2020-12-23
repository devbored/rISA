cmake_minimum_required(VERSION 3.10)

project(ricestick)
add_custom_target(EXAMPLES ALL
    COMMAND cmake ${CMAKE_SOURCE_DIR}/examples -Bexamples -G ${CMAKE_GENERATOR}
    COMMAND cmake --build examples
)
add_custom_target(RIEMU ALL
    COMMAND cmake ${CMAKE_SOURCE_DIR}/riemu -Briemu -G ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    COMMAND cmake --build riemu
)
