// error_handler_test.cpp

#include "utilities/error_handler.hpp"
#include "utilities/logger.hpp"
#include <iostream>
#include <cassert>
#include <fstream>
#include <cstdio>

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

void test_logger_connection() {
    std::cout << "Running test_logger_connection..." << std::endl;

    const std::string logFileName = "test_error_handler.log";

    // Clean up any pre-existing log file
    std::remove(logFileName.c_str());

    // Inject a test logger
    ErrorHandler::instance().setLogger(std::make_unique<Logger>(logFileName));

    // Log some messages
    ErrorHandler::instance().info("Info message for file");
    ErrorHandler::instance().warning("Warning message for file");
    ErrorHandler::instance().error("Error message for file");

    // Reset the logger to default/null to flush and close the file
    ErrorHandler::instance().setLogger(nullptr);

    // Now open the log file and verify the contents
    std::ifstream logFile(logFileName);
    assert(logFile.is_open() && "Log file was not created");

    std::string line;
    bool foundInfo = false;
    bool foundWarning = false;
    bool foundError = false;

    while (std::getline(logFile, line)) {
        if (line.find("[INFO] Info message for file") != std::string::npos) {
            foundInfo = true;
        }
        if (line.find("[WARNING] Warning message for file") != std::string::npos) {
            foundWarning = true;
        }
        if (line.find("[ERROR] Error message for file") != std::string::npos) {
            foundError = true;
        }
    }
    logFile.close();

    assert(foundInfo && "INFO message not logged to file");
    assert(foundWarning && "WARNING message not logged to file");
    assert(foundError && "ERROR message not logged to file");

    // Clean up the log file
    std::remove(logFileName.c_str());

    std::cout << "Passed: test_logger_connection\n" << std::endl;
}

int main() {
    test_info_warning_error_methods();
    test_custom_callback();
    test_handle_system_error();
    test_logger_connection();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
