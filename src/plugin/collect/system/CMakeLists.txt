project(system_collector)
include_directories(command)
add_custom_target(ebpf_build
        COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_CURRENT_SOURCE_DIR}/net_interface/ebpf
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/net_interface/ebpf
        COMMENT "Building eBPF programs using existing Makefile"
        SOURCES net_interface/ebpf/Makefile net_interface/ebpf/Makefile.arch net_interface/ebpf/net_flow_kernel.c  # 声明依赖文件
        )
add_library(system_collector SHARED
            thread_collector.cpp
            system_collector.cpp
            kernel_config.cpp
            kernel_data.cpp
            env_info.cpp
            ./command/command_collector.cpp
            ./command/command_base.cpp
            # net intf
            ./net_interface/net_interface.cpp
            ./net_interface/net_intf_comm.cpp
            )
target_include_directories(system_collector PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(system_collector PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
# 建立依赖关系
add_dependencies(system_collector ebpf_build)

# 包含头文件路径
target_include_directories(system_collector PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/net_interface/ebpf   # 假设Makefile生成头文件在此目录
        ${CMAKE_CURRENT_BINARY_DIR}/net_interface/ebpf   # 如果生成文件在构建目录
        )

if (WITH_ASAN)
    enable_asan(system_collector)
endif()
target_link_libraries(system_collector bpf)
target_link_libraries(system_collector common)
target_link_libraries(system_collector numa)
set_target_properties(system_collector PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_LIBRARY_DIRECTORY})
