project(analysis)
message("-- libkperf library path: ${LIB_KPERF_LIBPATH}")
message("-- libkperf include path: ${LIB_KPERF_INCPATH}")

set(analysis_src
    analysis/analysis.cpp
    analysis/common.cpp
    analysis/env.cpp
    hugepage/hugepage_analysis.cpp
    dynamic_smt/dynamic_smt_analysis.cpp
    smc_d_scenario/smc_d_analysis.cpp
    xcall/xcall_analysis.cpp
    net_hirq/net_hirq_analysis.cpp
    numa_analysis/numa_analysis.cpp
    microarch_tidnocmp/microarch_tidnocmp_analysis.cpp
    common/analysis_utils.cpp
)

set(oeaware_src analysis_oeaware.cpp)

add_library(analysis_lib OBJECT ${analysis_src})

target_include_directories(analysis_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${LIB_KPERF_INCPATH}
    ../../../common
    /usr/include/libnl3
)

target_link_libraries(analysis_lib numa boundscheck nl-3 nl-genl-3)

add_library(analysis_oeaware SHARED ${oeaware_src})

if (WITH_ASAN)
    enable_asan(analysis_oeaware)
endif()

target_link_libraries(analysis_oeaware analysis_lib)
set_target_properties(analysis_oeaware PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})