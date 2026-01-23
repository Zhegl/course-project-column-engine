add_library(types_lib
    types.cpp
)

target_include_directories(types_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)
