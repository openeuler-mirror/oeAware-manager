cmake_minimum_required(VERSION 3.11)
project(system_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra)

include_directories(../include)
add_subdirectory(cpu/stealtask_tune)
add_subdirectory(network/smc_tune)
add_subdirectory(power/seep_tune)
add_library(system_tune SHARED
        system_tune.cpp)

target_link_libraries(system_tune stealtask_tune smc_tune seep_tune)
set_target_properties(system_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})