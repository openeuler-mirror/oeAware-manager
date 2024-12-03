cmake_minimum_required(VERSION 3.11)
project(thread_scenario)
add_compile_options(-O2 -fPIC -Wall -Wextra)
add_library(thread_scenario SHARED
    thread_aware.cpp
)
target_include_directories(thread_scenario PUBLIC ${CMAKE_SOURCE_DIR}/include)
set_target_properties(thread_scenario PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/thread_scenario.conf"
                      DESTINATION "${PLUGIN_OUTPUT_LIBRARY_DIRECTORY}")