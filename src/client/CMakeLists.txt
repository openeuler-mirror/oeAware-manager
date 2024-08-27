cmake_minimum_required (VERSION 3.16)
project(oeAware-client)

SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++11 -g -Wl,-z,relro,-z,now -O2 -Wall -Wextra")

aux_source_directory(. SOURCE)
include_directories(../common)
link_directories(/usr/lib64)

add_executable (oeawarectl
                ${SOURCE}
                ) 
target_link_libraries(oeawarectl common boundscheck)