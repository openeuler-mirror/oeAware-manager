
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/collect/include
    ${CMAKE_CURRENT_SOURCE_DIR}/scenario/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tune/include
)

set(PLUGIN_OUTPUT_LIBRARY_DIRECTORY ${CMAKE_BINARY_DIR}/output/plugin/lib)
set(PLUGIN_OBJECT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/plugin/ko)
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
file(MAKE_DIRECTORY ${PLUGIN_OBJECT_OUTPUT_DIRECTORY})

add_subdirectory(collect/pmu)
add_subdirectory(collect/thread_collector)

add_subdirectory(scenario/analysis)
add_subdirectory(scenario/thread_aware)

add_subdirectory(tune/smc_tune)
add_subdirectory(tune/stealtask_tune)
add_subdirectory(tune/thread_tune)