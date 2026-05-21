#include "core/file_system.hpp"
#include <iostream>
#include <fstream>
#include "utilities/error_handler.hpp"
#include "utilities/logger.hpp"
#include <cstdlib> // for std::abort

inline void test_assert(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << std::endl;
        std::abort();
    }
}

int main() {
    std::filesystem::path testPath = "example.txt";

    // Create the file
    std::ofstream file(testPath);
    file << "Test content.\n";
    file.close();

    // Test existence again
    std::cout << "Exists after creation: " << FileSystem::exists(testPath) << std::endl;

    // Clean up
    std::filesystem::remove(testPath);

    // Negative Test: Reading a non-existent file to trigger the error path in readFile
    std::cout << "Running negative test: reading non-existent file..." << std::endl;
    
    // Inject a local logger with an absolute path for local test verification
    const std::string logFileName = std::filesystem::absolute("test_file_system_only.log").string();
    std::remove(logFileName.c_str());
    
    ErrorHandler::instance().setLogger(std::make_unique<Logger>(logFileName));
    
    auto content = FileSystem::readFile("/non/existent/file/path/that/fails.txt");
    test_assert(!content.has_value(), "Content must not be available for non-existent file");

    // Reset logger to flush and close file before reading it
    ErrorHandler::instance().setLogger(nullptr);

    // Verify error was logged to the test file
    std::ifstream logFile(logFileName);
    bool foundLoggedError = false;
    if (logFile.is_open()) {
        std::string line;
        while (std::getline(logFile, line)) {
            if (line.find("Error opening file for reading") != std::string::npos) {
                foundLoggedError = true;
                break;
            }
        }
        logFile.close();
    }
    
    std::cout << "Negative test status: " << (foundLoggedError ? "PASSED" : "FAILED") << std::endl;
    test_assert(foundLoggedError, "Expected error log 'Error opening file for reading' in test log file not found!");

    // Clean up the log file
    std::remove(logFileName.c_str());

    std::cout << "All file system tests completed successfully!" << std::endl;
    return 0;
}
