project(ub_tune)

add_library(ub_tune SHARED
        ub_tune.cpp
)

target_link_libraries(ub_tune numa)

if (WITH_ASAN)
    enable_asan(ub_tune)
endif()

set_target_properties(ub_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/ub_tune.conf"
            DESTINATION "${PLUGIN_OUTPUT_LIBRARY_DIRECTORY}")