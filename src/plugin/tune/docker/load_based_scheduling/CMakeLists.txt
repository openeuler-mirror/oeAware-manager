project(load_based_scheduling_tune)

add_library(load_based_scheduling_tune STATIC
            load_based_scheduling.cpp
)

target_link_libraries(load_based_scheduling_tune common)
target_include_directories(load_based_scheduling_tune PUBLIC
    ${LIB_KPERF_INCPATH}
)