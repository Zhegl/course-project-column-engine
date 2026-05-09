add_library(engine_lib
    engine.cpp
    queries.cpp
    api.cpp
    parser.cpp
)

target_include_directories(engine_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_lib
    PUBLIC
        glog::glog
)

target_link_libraries(engine_lib
    PUBLIC
        format_lib
        io_lib
        types_lib
)
