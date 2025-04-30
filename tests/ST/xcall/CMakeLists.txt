cmake_minimum_required(VERSION 3.16)
add_compile_options(-g -fPIC -Wall -Wextra -O2)
add_executable(test_xcall
    main.cpp
)

target_link_libraries(test_xcall PUBLIC oeaware-sdk)
target_include_directories(test_xcall PUBLIC
    ${SDK_INC_PATH}
)

set_target_properties(test_xcall PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests/")