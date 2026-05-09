add_library(convert_lib
    convert.cpp
)

target_include_directories(convert_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(convert_lib
    PUBLIC
        glog::glog
)

target_link_libraries(convert_lib
    PUBLIC
        format_lib
        io_lib
        types_lib
)
