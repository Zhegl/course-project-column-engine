add_library(types_lib
    types.cpp
)

target_include_directories(types_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(types_lib
    PUBLIC
        interface
)
