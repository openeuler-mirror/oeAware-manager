cmake_minimum_required(VERSION 3.11)
project(stealtask_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra)

add_library(stealtask_tune STATIC
        stealtask_tune.cpp
)

target_link_libraries(stealtask_tune numa)