project(smc_tune)

include_directories(/usr/include/libnl3)

add_subdirectory(kprobe)

add_library(smc_tune STATIC
        smc_tune.cpp
        smc_ueid.cpp
        smc_netlink.cpp)

target_link_libraries(smc_tune nl-genl-3 nl-3)
