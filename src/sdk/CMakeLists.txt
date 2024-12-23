project(oeaware-sdk)

aux_source_directory(. SOURCE)

add_library(${PROJECT_NAME} SHARED
            oe_client.cpp
)

if (WITH_ASAN)
    enable_asan(${PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
target_link_libraries(${PROJECT_NAME} common)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/oe_client.h"
DESTINATION "${CMAKE_BINARY_DIR}/output/include/oeaware")

file(MAKE_DIRECTORY ${SDK_OUTPUT_LIBRARY_DIRECTORY})
set_target_properties(${PROJECT_NAME} PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${SDK_OUTPUT_LIBRARY_DIRECTORY})