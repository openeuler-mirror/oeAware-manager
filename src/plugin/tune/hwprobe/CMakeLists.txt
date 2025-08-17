project(hwprobe_tune)

add_subdirectory(ext/hwprobe_ext_zbb_tune)

add_library(hwprobe_tune SHARED
        hwprobe_tune.cpp)

if (WITH_ASAN)
    enable_asan(hwprobe_tune)
endif()

target_link_libraries(hwprobe_tune hwprobe_ext_zbb_tune)

set_target_properties(hwprobe_tune PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})