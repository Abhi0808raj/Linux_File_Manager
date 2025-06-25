#include "error_handler.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>

// =======================
// FileManagerException Implementation
// =======================

// Copy constructor that takes string by reference. Used to pass message in a string
FileManagerException::FileManagerException(const std::string& message)
    : m_message(message) {}

// Move constructor for handling temporary strings for efficiency
FileManagerException::FileManagerException(std::string&& message)
    : m_message(std::move(message)) {}

// tells the compiler to not throw exception and return message
const char* FileManagerException::what() const noexcept {
    return m_message.c_str();
}

const std::string& FileManagerException::message() const noexcept {
    return m_message;
}

// =======================
// Derived Exception Implementations
// =======================

FileSystemException::FileSystemException(const std::string& message)
    : FileManagerException("FileSystem Error: " + message) {}

PluginException::PluginException(const std::string& message)
    : FileManagerException("Plugin Error: " + message) {}

GuiException::GuiException(const std::string& message)
    : FileManagerException("GUI Error: " + message) {}


// =======================
// ErrorHandler Implementation
// =======================

// Singleton instance ensuring only one instance of error
// for easy management
ErrorHandler& ErrorHandler::instance() {
    static ErrorHandler instance;
    return instance;
}

// Private constructor
ErrorHandler::ErrorHandler() = default;
ErrorHandler::~ErrorHandler() = default;

// Thread safe Custom logging and stores that log in
// errorCallback which is to be used in logError
void ErrorHandler::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    errorCallback_ = callback;
}

// Core method for logging errors
void ErrorHandler::logError(ErrorSeverity severity, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* severityStr = "";
    switch (severity) {
        case ErrorSeverity::WARNING: severityStr = "WARNING"; break;
        case ErrorSeverity::ERROR: severityStr = "ERROR"; break;
        case ErrorSeverity::CRITICAL: severityStr = "CRITICAL"; break;
    }

    // Default logging to stderr
    std::cerr << "[" << severityStr << "] " << message << std::endl;

    // If custom handler is there use it
    if (errorCallback_) {
        errorCallback_(severity, message);
    }
}

// Handling the Errors
// and Mapping to appropriate exceptions
void ErrorHandler::handleSystemError(const std::string& operation) {
    std::string errorMsg = operation + " failed: " + std::strerror(errno);
    instance().logError(ErrorSeverity::ERROR, errorMsg);

    // these errno come from library itself so no need to specify manually
    switch (errno) {
        case ENOENT:
            throw FileSystemException("File or directory not found: " + operation);
        case EACCES:
            throw FileSystemException("Permission denied: " + operation);
        case ENOMEM:
            throw FileManagerException("Out of memory: " + operation);
        default:
            throw FileSystemException(errorMsg);
    }
}