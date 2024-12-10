cmake_minimum_required(VERSION 3.11)
project(xcall_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra -Wno-unused-value -Wno-missing-field-initializers)

add_library(xcall_tune STATIC
    xcall_tune.cpp
)

target_link_libraries(xcall_tune common)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/xcall.yaml"
                      DESTINATION "${PLUGIN_OUTPUT_LIBRARY_DIRECTORY}")
