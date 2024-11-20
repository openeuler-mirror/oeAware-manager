cmake_minimum_required(VERSION 3.11)
project(smc_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra -Wno-unused-value -Wno-missing-field-initializers)

include_directories(/usr/include/libnl3)

add_subdirectory(kprobe)

add_library(smc_tune STATIC
        smc_tune.cpp
        smc_ueid.cpp
        smc_netlink.cpp)

target_link_libraries(smc_tune nl-genl-3 nl-3)
