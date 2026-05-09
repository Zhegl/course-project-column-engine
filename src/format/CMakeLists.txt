add_library(format_lib
    schema_reader.cpp
    meta_reader.cpp
)

target_include_directories(format_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(format_lib
    PUBLIC
        glog::glog
)

target_link_libraries(format_lib
    PUBLIC
        io_lib
        types_lib
)
