add_executable(test_convert
    test_convert.cpp
)
target_link_libraries(test_convert
    PRIVATE
        convert_lib
        gtest
        glog::glog
)


add_executable(test_writer_reader
    test_writer_reader.cpp
)
target_link_libraries(test_writer_reader
    PRIVATE
        convert_lib
        gtest
        glog::glog
)
