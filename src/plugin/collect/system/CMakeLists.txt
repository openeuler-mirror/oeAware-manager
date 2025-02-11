project(system_collector)
include_directories(command)
add_library(system_collector SHARED
            thread_collector.cpp
            system_collector.cpp
            kernel_config.cpp
            kernel_data.cpp
            env_info.cpp
            ./command/command_collector.cpp
            ./command/command_base.cpp
            )
target_include_directories(system_collector PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(system_collector PRIVATE ${CMAKE_SOURCE_DIR}/src/common)

if (WITH_ASAN)
    enable_asan(system_collector)
endif()

target_link_libraries(system_collector common)
target_link_libraries(system_collector numa)
set_target_properties(system_collector PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
