cmake_minimum_required(VERSION 3.16)

project(oeAware-manager)

set(SDK_OUTPUT_LIBRARY_DIRECTORY ${CMAKE_BINARY_DIR}/output/sdk)
set(SDK_INC_PATH ${CMAKE_CURRENT_LIST_DIR}/src/sdk)

add_subdirectory(src/common)
add_subdirectory(src/plugin)
add_subdirectory(src/plugin_mgr)
add_subdirectory(src/client)
add_subdirectory(src/sdk)
add_subdirectory(tests)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
file(COPY "${CMAKE_SOURCE_DIR}/include/oeaware/data/pmu_counting_data.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/data/pmu_sampling_data.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/data/pmu_spe_data.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/data/pmu_uncore_data.h"   
    "${CMAKE_SOURCE_DIR}/include/oeaware/data/docker_data.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/data/analysis_data.h"
    DESTINATION "${CMAKE_BINARY_DIR}/output/include/oeaware/data")
endif()
file(COPY "${CMAKE_SOURCE_DIR}/include/oeaware/data/kernel_data.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/data/command_data.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/data/thread_info.h"
    DESTINATION "${CMAKE_BINARY_DIR}/output/include/oeaware/data")

file(COPY "${CMAKE_SOURCE_DIR}/include/oeaware/data_list.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/default_path.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/instance_run_message.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/interface.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/logger.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/safe_queue.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/serialize.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/topic.h"
    "${CMAKE_SOURCE_DIR}/include/oeaware/utils.h"
    DESTINATION "${CMAKE_BINARY_DIR}/output/include/oeaware/")