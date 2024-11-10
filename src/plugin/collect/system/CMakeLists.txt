cmake_minimum_required(VERSION 3.11)
project(system_collector)
include_directories(../include)
add_compile_options(-O2 -fPIC -Wall -Wextra)
add_library(system_collector SHARED
            thread_collector.cpp
            system_collector.cpp
            kernel_config.cpp
            kernel_data.cpp
            ./command/command_collector.cpp
            ./command/mpstat_collector.cpp
            ./command/iostat_collector.cpp
            ./command/vmstat_collector.cpp
            )
set_target_properties(system_collector PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})