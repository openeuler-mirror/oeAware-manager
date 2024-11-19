cmake_minimum_required(VERSION 3.11)

project(docker_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra)

message("-- libkperf include path: ${LIB_KPERF_INCPATH}")

add_library(docker_tune SHARED
            cpu_burst.cpp
            cpu_burst_adapt.cpp
            docker_adapt.cpp
)

include_directories(docker_tune PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
    ${LIB_KPERF_INCPATH}
)

set_target_properties(docker_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})