#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

// Constructor that opens the log file
Logger::Logger(const std::string& filename)
    : logFile(filename, std::ios::app) // open file in append mode
{
    // fallback in case file fails to open
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

// Destructor closes the file
Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

// Logs the message with time and severity
void Logger::log(ErrorSeverity severity, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream logStream;

    // get current time
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    logStream << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "] ";

    // add severity label
    switch (severity) {
    case ErrorSeverity::WARNING:
        logStream << "[WARNING] ";
        break;
    case ErrorSeverity::ERROR:
        logStream << "[ERROR] ";
        break;
    case ErrorSeverity::CRITICAL:
        logStream << "[CRITICAL] ";
        break;
    }

    // add message and newline
    logStream << message << std::endl;

    // write to file or fallback to cerr
    if (logFile.is_open()) {
        logFile << logStream.str();
        logFile.flush();
    } else {
        std::cerr << logStream.str();
    }
}
