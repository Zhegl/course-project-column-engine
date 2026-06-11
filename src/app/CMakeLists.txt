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

add_executable(run_query
    run_query.cpp
)

target_link_libraries(run_query
    PRIVATE
        api_lib
        glog::glog
)
