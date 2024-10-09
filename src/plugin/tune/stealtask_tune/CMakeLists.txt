cmake_minimum_required(VERSION 3.11)
project(stealtask_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra)

include_directories(../include)

add_library(stealtask_tune SHARED
    stealtask_tune.cpp
)

target_link_libraries(stealtask_tune numa)
set_target_properties(stealtask_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})