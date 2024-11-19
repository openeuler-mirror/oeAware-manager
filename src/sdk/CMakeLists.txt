cmake_minimum_required (VERSION 3.16)
project(oeaware-sdk)

aux_source_directory(. SOURCE)
SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++14 -g -Wl,-z,relro,-z,now -Wall -Wextra -fPIC -O2")

include_directories(../common
    ${LIB_KPERF_INCPATH}
    ../plugin/collect/include
	../plugin/scenario/include    
)

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
set(ARCH_SPECIFIC_SOURCES
)
endif()

add_library(${PROJECT_NAME} SHARED
            oe_client.cpp
            ../common/data_register.cpp
            ${ARCH_SPECIFIC_SOURCES}
)
target_link_libraries(${PROJECT_NAME} common)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/oe_client.h"
DESTINATION "${CMAKE_BINARY_DIR}/include")