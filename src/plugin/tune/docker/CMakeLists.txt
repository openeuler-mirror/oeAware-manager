project(docker_tune)

add_library(docker_tune SHARED
            cpu_burst.cpp
            cpu_burst_adapt.cpp
            docker_adapt.cpp
)

include_directories(docker_tune PUBLIC
    ${LIB_KPERF_INCPATH}
)

if (WITH_ASAN)
    enable_asan(docker_tune)
endif()

set_target_properties(docker_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
