cmake_minimum_required(VERSION 3.16)
project(example_plugin)

add_library(example_plugin SHARED src/example_plugin.cpp)
target_include_directories(example_plugin
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(example_plugin PRIVATE file_manager_core)
set_target_properties(example_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
install(TARGETS example_plugin DESTINATION plugins)
