include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

set(PLUGIN_OUTPUT_LIBRARY_DIRECTORY ${CMAKE_BINARY_DIR}/output/plugin/lib)
set(PLUGIN_OBJECT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/plugin/ko)
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
file(MAKE_DIRECTORY ${PLUGIN_OBJECT_OUTPUT_DIRECTORY})

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|riscv64")
   add_subdirectory(collect/pmu)
   add_subdirectory(collect/docker)
   add_subdirectory(tune/docker)
endif()

add_subdirectory(collect/system)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|riscv64")
    add_subdirectory(scenario/analysis)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "riscv64")
    add_subdirectory(tune/hwprobe)
endif()

add_subdirectory(scenario/thread_aware)
add_subdirectory(tune/system)
add_subdirectory(tune/unixbench)