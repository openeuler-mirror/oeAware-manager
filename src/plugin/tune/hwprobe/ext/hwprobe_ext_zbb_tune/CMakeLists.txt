project(hwprobe_ext_zbb_tune)

add_library(hwprobe_ext_zbb_tune SHARED
    hwprobe_ext_zbb_tune.cpp
)

target_link_libraries(hwprobe_ext_zbb_tune dl)

if (WITH_ASAN)
    enable_asan(hwprobe_ext_zbb_tune)
endif()

set_target_properties(hwprobe_ext_zbb_tune PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})