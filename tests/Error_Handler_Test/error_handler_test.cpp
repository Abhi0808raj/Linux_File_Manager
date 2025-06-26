// error_handler_test.cpp

#include "utilities/error_handler.hpp"
#include <iostream>
#include <cassert>

void test_info_warning_error_methods() {
    std::cout << "Running test_info_warning_error_methods..." << std::endl;

    ErrorHandler::instance().info("Test info message");
    ErrorHandler::instance().warning("Test warning message");
    ErrorHandler::instance().error("Test error message");

    std::cout << "Passed: test_info_warning_error_methods\n" << std::endl;
}

void test_custom_callback() {
    std::cout << "Running test_custom_callback..." << std::endl;

    bool callbackTriggered = false;
    std::string receivedMessage;
    ErrorSeverity receivedSeverity = ErrorSeverity::WARNING;

    ErrorHandler::instance().setErrorCallback(
        [&](ErrorSeverity severity, const std::string& message) {
            callbackTriggered = true;
            receivedSeverity = severity;
            receivedMessage = message;
        }
    );

    ErrorHandler::instance().warning("Callback test warning");

    assert(callbackTriggered);
    assert(receivedSeverity == ErrorSeverity::WARNING);
    assert(receivedMessage == "Callback test warning");

    // 🛠️ Fix: clear callback to avoid dangling lambda
    ErrorHandler::instance().setErrorCallback(nullptr);

    std::cout << "Passed: test_custom_callback\n" << std::endl;
}

void test_handle_system_error() {
    std::cout << "Running test_handle_system_error..." << std::endl;

    // Try to open a non-existent file to set errno
    FILE* file = fopen("/non/existing/path.txt", "r");
    if (!file) {
        try {
            ErrorHandler::instance().handleSystemError("Open non-existing file");
        } catch (const FileSystemException& ex) {
            std::cout << "Caught expected FileSystemException: " << ex.what() << std::endl;
        } catch (...) {
            assert(false && "Unexpected exception type");
        }
    }

    std::cout << "Passed: test_handle_system_error\n" << std::endl;
}

int main() {
    test_info_warning_error_methods();
    test_custom_callback();
    test_handle_system_error();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
