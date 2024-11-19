cmake_minimum_required(VERSION 3.11)
project(docker_collector)
include_directories(../include)
add_compile_options(-O2 -fPIC -Wall -Wextra)
add_library(docker_collector SHARED
            docker_adapt.cpp
            docker_collector.cpp
            )
set_target_properties(docker_collector PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})