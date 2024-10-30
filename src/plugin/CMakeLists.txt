
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/collect/include
    ${CMAKE_CURRENT_SOURCE_DIR}/scenario/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tune/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../new_intf
)

set(PLUGIN_OUTPUT_LIBRARY_DIRECTORY ${CMAKE_BINARY_DIR}/output/plugin/lib)
set(PLUGIN_OBJECT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/plugin/ko)
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
file(MAKE_DIRECTORY ${PLUGIN_OBJECT_OUTPUT_DIRECTORY})

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
   add_subdirectory(collect/pmu)
endif()
# add_subdirectory(collect/system)

# if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
#     add_subdirectory(scenario/analysis)
# endif()
# add_subdirectory(scenario/thread_aware)

# add_subdirectory(tune/smc_tune)
# add_subdirectory(tune/system)
# add_subdirectory(tune/unixbench)