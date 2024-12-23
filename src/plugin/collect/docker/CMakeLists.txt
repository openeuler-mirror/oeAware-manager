project(docker_collector)
add_library(docker_collector SHARED
            docker_adapt.cpp
            docker_collector.cpp
            )
if (WITH_ASAN)
    enable_asan(docker_collector)
endif()
set_target_properties(docker_collector PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
