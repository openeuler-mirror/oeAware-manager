project(system_tune)

add_subdirectory(cpu/stealtask_tune)
add_subdirectory(cpu/dynamic_smt_tune)
add_subdirectory(cpu/numa_sched_tune)
add_subdirectory(network/smc_tune)
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    add_subdirectory(network/hardirq_tune)
    add_subdirectory(network/multi_net_path)
endif()
add_subdirectory(xcall)
add_subdirectory(power/seep_tune)
add_subdirectory(transparent_hugepage_tune)
add_subdirectory(preload)
add_subdirectory(binary)
add_subdirectory(realtime)

add_library(system_tune SHARED
        system_tune.cpp)

if (WITH_ASAN)
    enable_asan(system_tune)
endif()

target_link_libraries(system_tune stealtask_tune dynamic_smt_tune smc_tune xcall_tune numa_sched_tune seep_tune preload_tune transparent_hugepage_tune binary_tune realtime_tune)
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    target_compile_definitions(system_tune PRIVATE BUILD_NETIRQ_TUNE)
    target_compile_definitions(system_tune PRIVATE BUILD_MULTI_NET_PATH_TUNE)
    target_link_libraries(system_tune net_hardirq_tune multi_net_path)
endif()

set_target_properties(system_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})