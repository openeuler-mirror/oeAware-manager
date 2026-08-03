project(disk_adapt_ic)

add_library(disk_adapt_ic STATIC
        disk_adapt_ic.cpp)

target_include_directories(disk_adapt_ic PRIVATE
        ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(disk_adapt_ic yaml-cpp)

if (WITH_ASAN)
    enable_asan(disk_adapt_ic)
endif()
