#include "utilities/logger.hpp"  // Adjust the path as per your structure
#include <iostream>

int main() {
    // Create a logger instance (log file will be created in current directory)
    Logger logger("test_log.txt");

    // Log messages of different severity
    logger.log(ErrorSeverity::WARNING, "This is a warning message.");
    logger.log(ErrorSeverity::ERROR, "This is an error message.");
    logger.log(ErrorSeverity::CRITICAL, "This is a critical message.");

    std::cout << "Logging completed. Check test_log.txt for output.\n";
    return 0;
}
