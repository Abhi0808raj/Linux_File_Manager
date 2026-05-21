# add_test (not gtest_discover_tests): copy_plugin.so must exist at test run time.
add_executable(test_copy_plugin_integration test_copy_plugin.cpp)
target_link_libraries(test_copy_plugin_integration PRIVATE
    file_manager_core GTest::gtest_main dl
)
target_compile_definitions(test_copy_plugin_integration PRIVATE
    PLUGIN_BUILD_DIR="${PLUGIN_BUILD_DIR}"
)
add_dependencies(test_copy_plugin_integration copy_plugin)
add_test(NAME CopyPluginIntegration COMMAND test_copy_plugin_integration)
