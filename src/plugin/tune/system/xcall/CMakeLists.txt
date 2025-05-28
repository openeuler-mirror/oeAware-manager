project(xcall_tune)

add_library(xcall_tune STATIC
    xcall_tune.cpp
)

target_link_libraries(xcall_tune common)
