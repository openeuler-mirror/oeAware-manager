project(xcall_tune)

add_library(xcall_tune STATIC
    xcall_tune.cpp
)

target_link_libraries(xcall_tune common)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/xcall.yaml"
                      DESTINATION "${PLUGIN_OUTPUT_LIBRARY_DIRECTORY}")
