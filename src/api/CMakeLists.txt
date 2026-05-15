add_library(api_lib
    columnar_engine.cpp
)

target_include_directories(api_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(api_lib
    PUBLIC
        engine_lib
)
