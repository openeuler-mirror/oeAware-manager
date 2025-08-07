project(thread_scenario)

add_library(thread_scenario SHARED
    thread_aware.cpp
)
target_include_directories(thread_scenario PUBLIC ${CMAKE_SOURCE_DIR}/include)

if (WITH_ASAN)
    enable_asan(thread_scenario)
endif()

set_target_properties(thread_scenario PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
