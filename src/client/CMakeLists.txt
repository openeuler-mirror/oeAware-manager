project(oeAware-client)

add_subdirectory(analysis)

aux_source_directory(. SOURCE)
add_executable(oeawarectl ${SOURCE})
target_include_directories(oeawarectl PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis
                ${CMAKE_CURRENT_SOURCE_DIR}/../common
            )

if (WITH_ASAN)
    enable_asan(oeawarectl)
endif()

target_link_libraries(oeawarectl analysis_cli common boundscheck)
set_target_properties(oeawarectl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/bin")
