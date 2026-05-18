add_executable(test_engine
    test_engine.cpp
)
target_link_libraries(test_engine
    PRIVATE
        api_lib
        convert_lib
        gtest
        glog::glog
)

add_executable(test_clickbench
    test_clickbench.cpp
)
target_link_libraries(test_clickbench
    PRIVATE
        api_lib
        convert_lib
        gtest
        glog::glog
)

add_executable(test_clickbench_auto
    test_clickbench_auto.cpp
)
target_include_directories(test_clickbench_auto
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_clickbench_auto
    PRIVATE
        api_lib
        gtest
        glog::glog
)

add_executable(test_multithreading
    test_multithreading.cpp
)
target_link_libraries(test_multithreading
    PRIVATE
        api_lib
        convert_lib
        gtest
        glog::glog
)

add_custom_command(TARGET test_clickbench_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_CURRENT_SOURCE_DIR}/check_query.py
        $<TARGET_FILE_DIR:test_clickbench_auto>/check_query.py
    COMMENT "Symlinking check_query.py to build/bin"
)
