cmake_minimum_required (VERSION 3.22)
project(oeAware-server)

SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++11 -g -Wl,-z,relro,-z,now")

if("${OEAWARE_DEBUG}" EQUAL 1)
add_definitions(-DOEAWARE_DEBUG)
endif()

aux_source_directory(. SOURCE)
include_directories(/usr/include)
include_directories(../common)
link_directories(/usr/lib64)

add_executable (oeAware
                ${SOURCE}
                ) 
target_link_libraries(oeAware common)
