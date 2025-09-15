project(docker_cluster_affinity_tune)

add_library(docker_cluster_affinity_tune STATIC
        topology.cpp
        cluster_selector.cpp
        cluster_affinity_adapt.cpp
)

target_include_directories(docker_cluster_affinity_tune PUBLIC
    ${LIB_KPERF_INCPATH}
)