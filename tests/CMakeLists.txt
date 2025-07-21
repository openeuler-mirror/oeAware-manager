project(test)
set(SRC_DIR ../src)

find_package(GTest CONFIG REQUIRED)

include_directories(../src/common)
include_directories(../include)
include_directories(../src/sdk)

add_executable(serialize_test
    ${SRC_DIR}/common/utils.cpp
    serialize_test.cpp
)

add_executable(logger_test
    ${SRC_DIR}/plugin_mgr/logger.cpp
    logger_test.cpp
)

add_executable(safe_queue_test
    safe_queue_test.cpp
)

add_executable(pmu_count_test
    pmu_count_test.cpp
)

add_executable(utils_test
    utils_test.cpp
)

add_executable(data_register_test
    data_register_test.cpp
)

add_executable(table_test
    table_test.cpp
    ${SRC_DIR}/client/analysis/table.cpp
)

add_executable(analysis_report_test
    analysis_report_test.cpp
    ${SRC_DIR}/client/analysis/config.cpp
    ${SRC_DIR}/client/analysis/analysis_report.cpp
    ${SRC_DIR}/client/analysis/table.cpp
)

target_include_directories( analysis_report_test PUBLIC
    ${SRC_DIR}/client/analysis
)

target_include_directories(table_test PUBLIC
    ${SRC_DIR}/client/analysis
)

target_include_directories(logger_test PUBLIC
    ${SRC_DIR}/plugin_mgr
)

target_link_libraries(serialize_test PRIVATE common GTest::gtest_main)
target_link_libraries(logger_test PRIVATE GTest::gtest_main log4cplus bpf)
target_link_libraries(safe_queue_test PRIVATE GTest::gtest_main)
target_link_libraries(pmu_count_test PRIVATE GTest::gtest_main)
target_link_libraries(utils_test PRIVATE common GTest::gtest_main)
target_link_libraries(data_register_test PRIVATE common GTest::gtest_main)
target_link_libraries(table_test PRIVATE common GTest::gtest_main)
target_link_libraries(analysis_report_test PRIVATE common oeaware-sdk GTest::gtest_main)

set_target_properties(serialize_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests")
set_target_properties(logger_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests")
set_target_properties(safe_queue_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests")
set_target_properties(utils_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests")
set_target_properties(data_register_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests")
set_target_properties(table_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests")
set_target_properties(analysis_report_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests")

add_subdirectory(ST/sdk)
add_subdirectory(ST/xcall)