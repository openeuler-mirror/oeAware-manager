project(docker_tune)
add_subdirectory(cpu_burst)
add_subdirectory(coordination_burst)

add_library(docker_tune SHARED
            docker_adapt.cpp)

if (WITH_ASAN)
    enable_asan(docker_tune)
endif()

target_link_libraries(docker_tune docker_cpu_burst_tune docker_coordination_burst_tune)
set_target_properties(docker_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
