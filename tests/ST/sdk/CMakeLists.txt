set(test_sdk_src
    test_env_info.cpp
    test_net_intf.cpp
    main.cpp
)

add_compile_options(-g -fPIC -Wall -Wextra -O2)
add_executable(test_sdk 
    ${test_sdk_src}
)

target_link_libraries(test_sdk PUBLIC oeaware-sdk)
target_include_directories(test_sdk PUBLIC
    ${SDK_INC_PATH}
)

set_target_properties(test_sdk PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/tests/")