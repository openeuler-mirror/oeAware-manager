project(system_tune)

add_subdirectory(cpu/stealtask_tune)
add_subdirectory(network/smc_tune)
add_subdirectory(network/hardirq_tune)
add_subdirectory(xcall)
add_subdirectory(power/seep_tune)
add_subdirectory(preload)

add_library(system_tune SHARED
        system_tune.cpp)

if (WITH_ASAN)
    enable_asan(system_tune)
endif()
target_link_libraries(system_tune stealtask_tune smc_tune net_hardirq_tune xcall_tune seep_tune preload_tune)

set_target_properties(system_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})