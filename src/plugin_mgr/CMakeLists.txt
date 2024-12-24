project(oeAware-server)

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
target_link_libraries(oeaware common)

if (WITH_ASAN)
    enable_asan(oeaware)
endif()

if (${CMAKE_CXX_COMPILER_VERSION} LESS "10.3.1")
    target_link_libraries(oeaware -lpthread -ldl)
endif()

set_target_properties(oeaware PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/bin")
