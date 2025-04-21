project(net_hardirq_tune)

add_library(net_hardirq_tune STATIC
        irq_frontend.cpp
        hardirq_tune.cpp
)

target_include_directories(net_hardirq_tune PUBLIC
    ${LIB_KPERF_INCPATH}
)