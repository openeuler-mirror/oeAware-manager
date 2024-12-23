cmake_minimum_required(VERSION 3.11)
project(stealtask_tune)

add_library(stealtask_tune STATIC
        stealtask_tune.cpp
)

target_link_libraries(stealtask_tune numa)