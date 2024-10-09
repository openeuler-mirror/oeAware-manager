cmake_minimum_required(VERSION 3.11)
project(thread_collector)
include_directories(../include)
add_compile_options(-O2 -fPIC -Wall -Wextra)
add_library(thread_collector SHARED
    thread_collector.cpp
)
set_target_properties(thread_collector PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})