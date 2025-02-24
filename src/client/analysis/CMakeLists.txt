set(analysis_cli_src
    analysis_cli.cpp
    config.cpp
    analysis_report.cpp
    table.cpp
)

add_compile_options(-g -fPIC -Wall -Wextra -O2)
add_library(analysis_cli OBJECT ${analysis_cli_src})

target_link_libraries(analysis_cli PUBLIC oeaware-sdk)
target_include_directories(analysis_cli PUBLIC
    ${SDK_INC_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)
