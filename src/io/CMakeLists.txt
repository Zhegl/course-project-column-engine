add_library(io_lib
    file_reader.cpp
    file_writer.cpp
)

target_include_directories(io_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(io_lib
    PUBLIC
        glog::glog
)