cmake_minimum_required (VERSION 3.22)
project(common)

aux_source_directory(. SOURCE)
SET(CMAKE_CXX_FLAGS "-rdynamic -std=c++11 -g -Wl,-z,relro,-z,now -Wall -Wextra -fPIC -O2")


include_directories(/usr/include/yaml-cpp)
include_directories(/usr/include/log4cplus)
include_directories(/usr/include/boost)
include_directories(/usr/include/curl)

link_directories(/usr/lib64)

add_library(${PROJECT_NAME}
            ${SOURCE}
)
target_link_libraries(${PROJECT_NAME} log4cplus)
target_link_libraries(${PROJECT_NAME} yaml-cpp)
target_link_libraries(${PROJECT_NAME} boost_serialization)
target_link_libraries(${PROJECT_NAME} curl)