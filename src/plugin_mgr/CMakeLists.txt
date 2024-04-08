cmake_minimum_required (VERSION 3.22)
project(oeAware-server)

aux_source_directory(. SOURCE)
include_directories(/usr/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
link_directories(/usr/lib64)


add_executable (oeAware
                ${SOURCE}
                ) 
target_link_libraries(oeAware  common)
