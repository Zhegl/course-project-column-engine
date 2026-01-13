cmake_minimum_required(VERSION 3.16)
project(columnar_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(cmake)
add_subdirectory(src)
add_subdirectory(tests)

enable_testing()