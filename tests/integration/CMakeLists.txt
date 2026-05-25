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

# FileView <-> MainWindow integration tests.
# Requires Qt6::Widgets and Qt6::Test (for QSignalSpy). file_manager_core
# already exports Qt6::Widgets transitively, but Qt6::Test is test-only.
add_executable(test_file_view_integration test_file_view_integration.cpp)
target_link_libraries(test_file_view_integration PRIVATE
    file_manager_core
    GTest::gtest_main
    Qt6::Test
)
gtest_discover_tests(test_file_view_integration)

# Context menu integration tests (Properties 4, 5, 8, 9, 10, 14).
# Requires Qt6::Widgets (via file_manager_core), Qt6::Test (QSignalSpy),
# and RapidCheck for property tests (tasks 12.2–12.7).
add_executable(test_context_menu_integration test_context_menu_integration.cpp)
target_link_libraries(test_context_menu_integration PRIVATE
    file_manager_core
    GTest::gtest_main
    Qt6::Test
    rapidcheck
    rapidcheck_gtest
)
gtest_discover_tests(test_context_menu_integration)
