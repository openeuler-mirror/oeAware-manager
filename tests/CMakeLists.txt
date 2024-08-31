cmake_minimum_required(VERSION 3.16)
project(test)
set(CMAKE_CXX_STANDARD 11)
set(SRC_DIR ../src)
SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++11 -g -Wl,-z,relro,-z,now")

find_package(GTest CONFIG REQUIRED)

include_directories(../src/common)

add_executable(serialize_test
    serialize_test.cpp
)       

target_link_libraries(serialize_test PRIVATE GTest::gtest_main)