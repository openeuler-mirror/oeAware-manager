cmake_minimum_required (VERSION 3.22)
project(oeAware-client)

include_directories(../common)
link_directories(/usr/lib64)

add_executable (oeawarectl
                cmd_handler.cpp
                main.cpp
                ) 
target_link_libraries(oeawarectl common)