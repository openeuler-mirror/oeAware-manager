cmake_minimum_required (VERSION 3.16)
project(oeAware-server)

SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++14 -g -Wl,-z,relro,-z,now -Wall -Wextra -O2")

if("${OEAWARE_DEBUG}" EQUAL 1)
add_definitions(-DOEAWARE_DEBUG)
endif()

aux_source_directory(. SOURCE)
aux_source_directory(event EVENT_SOURCE)
include_directories(/usr/include)
include_directories(../common)
include_directories(event)
include_directories(./)
link_directories(/usr/lib64)

add_executable (oeaware
                ${SOURCE}
                ${EVENT_SOURCE}
                ) 
target_link_libraries(oeaware common boundscheck)
if (${CMAKE_CXX_COMPILER_VERSION} LESS "10.3.1")
    target_link_libraries(oeaware -lpthread -ldl)
endif()
