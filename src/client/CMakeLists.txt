cmake_minimum_required (VERSION 3.16)
project(oeAware-client)

SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++14 -g -Wl,-z,relro,-z,now -O2 -Wall -Wextra")

add_subdirectory(analysis)

aux_source_directory(. SOURCE)
add_executable(oeawarectl ${SOURCE})
target_include_directories(oeawarectl PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis
                ${CMAKE_CURRENT_SOURCE_DIR}/../common
            )
target_link_libraries(oeawarectl analysis_cli common boundscheck)
set_target_properties(oeawarectl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/bin")
