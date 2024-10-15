cmake_minimum_required (VERSION 3.16)
project(sdk)

aux_source_directory(. SOURCE)
SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++14 -g -Wl,-z,relro,-z,now -Wall -Wextra -fPIC -O2")

include_directories(../common)
link_directories(/usr/lib64)

add_library(${PROJECT_NAME} SHARED
            ${SOURCE}
)
target_link_libraries(${PROJECT_NAME} common)