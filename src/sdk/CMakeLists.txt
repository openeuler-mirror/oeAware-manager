cmake_minimum_required (VERSION 3.16)
project(oeaware-sdk)

aux_source_directory(. SOURCE)
SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++14 -g -Wl,-z,relro,-z,now -Wall -Wextra -fPIC -O2")

add_library(${PROJECT_NAME} SHARED
            oe_client.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
target_link_libraries(${PROJECT_NAME} common)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/oe_client.h"
DESTINATION "${CMAKE_BINARY_DIR}/output/include/oeaware")

file(MAKE_DIRECTORY ${SDK_OUTPUT_LIBRARY_DIRECTORY})
set_target_properties(${PROJECT_NAME} PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${SDK_OUTPUT_LIBRARY_DIRECTORY})