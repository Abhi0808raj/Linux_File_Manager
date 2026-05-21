set(PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/plugins")
set(ROOT_INCLUDE "${CMAKE_SOURCE_DIR}/include")

# FileSystem tests (no Qt QApplication needed — uses std::filesystem only)
add_executable(test_filesystem test_file_system.cpp)
target_include_directories(test_filesystem PRIVATE ${ROOT_INCLUDE})
target_link_libraries(test_filesystem PRIVATE file_manager_core GTest::gtest_main)
gtest_discover_tests(test_filesystem)

# ErrorHandler tests
add_executable(test_error_handler test_error_handler.cpp)
target_include_directories(test_error_handler PRIVATE ${ROOT_INCLUDE})
target_link_libraries(test_error_handler PRIVATE file_manager_core GTest::gtest_main)
gtest_discover_tests(test_error_handler)

# Logger tests
add_executable(test_logger test_logger.cpp)
target_include_directories(test_logger PRIVATE ${ROOT_INCLUDE})
target_link_libraries(test_logger PRIVATE file_manager_core GTest::gtest_main)
gtest_discover_tests(test_logger)

# PluginManager tests — need .so files; use add_test for reliable ctest registration
add_executable(test_plugin_manager test_plugin_manager.cpp)
target_include_directories(test_plugin_manager PRIVATE ${ROOT_INCLUDE})
target_link_libraries(test_plugin_manager PRIVATE file_manager_core GTest::gtest_main dl)
target_compile_definitions(test_plugin_manager PRIVATE
    PLUGIN_BUILD_DIR="${PLUGIN_BUILD_DIR}"
)
add_dependencies(test_plugin_manager copy_plugin move_plugin delete_plugin)
add_test(NAME PluginManagerTests COMMAND test_plugin_manager)
