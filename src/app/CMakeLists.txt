add_executable(convert
    run_convert.cpp
)

target_link_libraries(convert
    PRIVATE
        convert_lib
)


target_link_libraries(convert_lib
    PUBLIC
        glog::glog
)
