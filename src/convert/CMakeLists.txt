add_library(convert_lib
    convert.cpp
    file_reader.cpp
    file_writer.cpp
    scheme_reader.cpp
)

target_include_directories(convert_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(convert_lib
    PUBLIC
        glog::glog
)

add_executable(convert
    run_convert.cpp
)

target_link_libraries(convert
    PRIVATE
        convert_lib
)