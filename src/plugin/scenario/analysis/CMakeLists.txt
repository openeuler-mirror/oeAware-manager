cmake_minimum_required(VERSION 3.15)

project(analysis)
message("-- libkperf library path: ${LIB_KPERF_LIBPATH}")
message("-- libkperf include path: ${LIB_KPERF_INCPATH}")
set(analysis_src
    analysis/analysis.cpp
    analysis/common.cpp
    analysis/env.cpp
)

set(oeaware_src adapt/oeaware_adapt.cpp)

add_compile_options(-g -fPIC -Wall -Wextra)

add_library(analysis_base OBJECT ${analysis_src})
target_include_directories(analysis_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis
    ${LIB_KPERF_INCPATH}
)


target_link_libraries(analysis_base numa)

add_library(analysis_oeaware SHARED ${oeaware_src})
target_include_directories(analysis_oeaware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(analysis_oeaware analysis_base)
set_target_properties(analysis_oeaware PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})


