cmake_minimum_required(VERSION 3.11)
project(ub_tune)

if (WITH_DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-fPIC -Wall -Wextra)

add_library(ub_tune SHARED
        ub_tune.cpp
)

target_link_libraries(ub_tune numa)
set_target_properties(ub_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/ub_tune.conf"
            DESTINATION "${PLUGIN_OUTPUT_LIBRARY_DIRECTORY}")