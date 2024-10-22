cmake_minimum_required(VERSION 3.15)
project(pmu_plugin)

set(CMAKE_CXX_STANDARD 14)

option(WITH_DEBUG "debug mode" OFF)

if (WITH_DEBUG)
    message("-- Note:pmu debug mode")
    add_compile_options(-g)
endif()
add_compile_options(-O2 -fPIC -Wall -Wextra)

# libkperf
message("-- libkperf library path: ${LIB_KPERF_LIBPATH}")
message("-- libkperf include path: ${LIB_KPERF_INCPATH}")

set(pmu_src
    pmu_counting_data.cpp
    pmu_counting_collector.cpp
    pmu_sampling_data.cpp
    pmu_sampling_collector.cpp
    pmu_spe_data.cpp
    pmu_spe_collector.cpp
    ../../../common/base_data.cpp
)

add_library(pmu SHARED ${pmu_src})

include_directories(pmu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
    ${LIB_KPERF_INCPATH}
)

target_link_directories(pmu PUBLIC
    ${LIB_KPERF_LIBPATH}
)

target_link_libraries(pmu kperf boundscheck)
set_target_properties(pmu PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
