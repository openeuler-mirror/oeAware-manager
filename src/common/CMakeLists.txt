project(common)

aux_source_directory(. SOURCE)

include_directories(/usr/include/log4cplus)
include_directories(../plugin/collect/include)
include_directories(../plugin/scenario/include)
include_directories(${LIB_KPERF_INCPATH})

add_library(${PROJECT_NAME}
            ${SOURCE}
)

target_link_libraries(${PROJECT_NAME} log4cplus bpf)
target_link_libraries(${PROJECT_NAME} yaml-cpp)
target_link_libraries(${PROJECT_NAME} curl boundscheck)

if (WITH_ASAN)
    enable_asan(${PROJECT_NAME})
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64|riscv64")
target_link_directories(${PROJECT_NAME} PUBLIC
    ${LIB_KPERF_LIBPATH}
)
target_link_libraries(${PROJECT_NAME} kperf)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
