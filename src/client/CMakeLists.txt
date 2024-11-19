cmake_minimum_required (VERSION 3.16)
project(oeAware-client)

SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++14 -g -Wl,-z,relro,-z,now -O2 -Wall -Wextra")

aux_source_directory(. SOURCE)
include_directories(../common)
include_directories(../sdk)
include_directories(../plugin/scenario/include)
link_directories(/usr/lib64)

add_executable (oeawarectl
                ${SOURCE}
                ) 
target_link_libraries(oeawarectl common boundscheck oeaware-sdk)
set_target_properties(oeawarectl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/bin")
