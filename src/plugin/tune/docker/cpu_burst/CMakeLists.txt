project(docker_cpu_burst_tune)

add_library(docker_cpu_burst_tune STATIC
            cpu_burst.cpp
            cpu_burst_adapt.cpp
)

target_include_directories(docker_cpu_burst_tune PUBLIC
    ${LIB_KPERF_INCPATH}
)