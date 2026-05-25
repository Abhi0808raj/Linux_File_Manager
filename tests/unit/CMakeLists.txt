# test_filesystem, test_error_handler, test_logger:
# Include dirs come transitively from file_manager_core (PUBLIC).

add_executable(test_filesystem test_file_system.cpp)
target_link_libraries(test_filesystem PRIVATE file_manager_core GTest::gtest_main)
gtest_discover_tests(test_filesystem)

add_executable(test_error_handler test_error_handler.cpp)
target_link_libraries(test_error_handler PRIVATE file_manager_core GTest::gtest_main)
gtest_discover_tests(test_error_handler)

add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE file_manager_core GTest::gtest_main)
gtest_discover_tests(test_logger)

# add_test (not gtest_discover_tests) because this target depends on .so files
# that are not guaranteed to exist at CMake configure/build time for POST_BUILD discovery.
add_executable(test_plugin_manager test_plugin_manager.cpp)
target_link_libraries(test_plugin_manager PRIVATE file_manager_core GTest::gtest_main dl)
target_compile_definitions(test_plugin_manager PRIVATE PLUGIN_BUILD_DIR="${PLUGIN_BUILD_DIR}")
add_dependencies(test_plugin_manager copy_plugin move_plugin delete_plugin)
add_test(NAME PluginManagerTests COMMAND test_plugin_manager)

# FileView unit + property-based tests (RapidCheck + GoogleTest)
# Source file created in task 8.2.
add_executable(test_file_view test_file_view.cpp)
target_link_libraries(test_file_view
    PRIVATE
        file_manager_core
        GTest::gtest_main
        rapidcheck
        rapidcheck_gtest
        Qt6::Test
)
gtest_discover_tests(test_file_view)
