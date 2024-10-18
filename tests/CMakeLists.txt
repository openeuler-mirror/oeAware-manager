cmake_minimum_required(VERSION 3.16)
project(test)
set(SRC_DIR ../src)
SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++14 -g -Wl,-z,relro,-z,now")

find_package(GTest CONFIG REQUIRED)

include_directories(../src/common)

add_executable(serialize_test
    ${SRC_DIR}/common/base_data.cpp
    ${SRC_DIR}/common/utils.cpp
    serialize_test.cpp
)

add_executable(logger_test
    ${SRC_DIR}/common/logger.cpp
    logger_test.cpp
)

add_executable(safe_queue_test
    safe_queue_test.cpp
)

add_executable(pmu_count_test
    ${SRC_DIR}/common/base_data.cpp
    pmu_count_test.cpp
)

target_link_libraries(serialize_test PRIVATE curl GTest::gtest_main)
target_link_libraries(logger_test PRIVATE GTest::gtest_main log4cplus)
target_link_libraries(safe_queue_test PRIVATE GTest::gtest_main)
target_link_libraries(pmu_count_test PRIVATE GTest::gtest_main)