add_executable(test_plugin_manager
        test_plugin_manager.cpp
)

target_include_directories(test_plugin_manager PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(test_plugin_manager
        PRIVATE file_manager_core
        PRIVATE dl
)

set_target_properties(test_plugin_manager PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Plugin_Manager_Test
)
add_dependencies(test_plugin_manager example_plugin)
