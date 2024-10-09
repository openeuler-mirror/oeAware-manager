cmake_minimum_required(VERSION 3.11)
project(thread_aware)
add_compile_options(-O2 -fPIC -Wall -Wextra)
include_directories(../include)
add_library(thread_scenario SHARED
    thread_aware.cpp
)

set_target_properties(thread_scenario PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})