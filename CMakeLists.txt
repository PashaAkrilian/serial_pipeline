cmake_minimum_required(VERSION 3.14)
project(serial_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)

add_library(serial_core
    src/serial_port.cpp
    src/frame_parser.cpp
    src/thread_pool.cpp
)
target_include_directories(serial_core PUBLIC include)
target_link_libraries(serial_core PUBLIC Threads::Threads)

add_executable(serial_demo app/main.cpp)
target_link_libraries(serial_demo PRIVATE serial_core)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(serial_tests
    tests/frame_parser_test.cpp
    tests/thread_safe_queue_test.cpp
    tests/thread_pool_test.cpp
    tests/logger_test.cpp
)
target_link_libraries(serial_tests PRIVATE serial_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(serial_tests)