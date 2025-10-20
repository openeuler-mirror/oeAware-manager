project(pmu_plugin)

set(pmu_src
    pmu_counting_collector.cpp
    pmu_sampling_collector.cpp
    pmu_spe_collector.cpp
    pmu_uncore_collector.cpp
    pmu_l3c_collector.cpp
    pmu_uncore.cpp
    pmu_collector.cpp
    pmu_common.cpp
)

add_library(pmu SHARED ${pmu_src})

if (WITH_ASAN)
    enable_asan(pmu)
endif()

target_link_libraries(pmu boundscheck kperf)
set_target_properties(pmu PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
