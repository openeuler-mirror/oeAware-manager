cmake_minimum_required(VERSION 3.11)
project(seep_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra)

include_directories(../include)
add_library(seep_tune STATIC
        seep_tune.cpp
)
