add_library(io_lib
    file_reader.cpp
    file_writer.cpp
)

target_include_directories(convert_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(convert_lib
    PUBLIC
        glog::glog
)