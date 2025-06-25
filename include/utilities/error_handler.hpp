#pragma once

#include <exception>
#include <string>
#include <sstream>        // for string based stream operations
#include <iostream>
#include <mutex>          // for thread safe operations
#include <functional>     // used to store custom error callback function
#include <cstring>        // provides C style string manipulation
#include <cerrno>         // for identifying cause of low level system errors

// Base Class for custom Exception
class FileManagerException : public std::exception {
protected:
    std::string m_message; // holds error message

public:
    // Copy constructor that takes string by reference. Used to pass message in a string
    FileManagerException(const std::string& message);

    // Move constructor for handling temporary strings for efficiency
    FileManagerException(std::string&& message);

    // tells the compiler to not throw exception and return message
    const char* what() const noexcept override;
    const std::string& message() const noexcept;
};

// Derived Exception types for specific modules
class FileSystemException : public FileManagerException {
public:
    FileSystemException(const std::string& message);
};

class PluginException : public FileManagerException {
public:
    PluginException(const std::string& message);
};

class GuiException : public FileManagerException {
public:
    GuiException(const std::string& message);
};

// class ErrorException : public FileManagerException
// {
// public:
//     ErrorException(const std::string& message)
//     : FileManagerException("Error: " + message) {}
// };

// Enum for categorizing error severity
enum class ErrorSeverity {
    WARNING,
    ERROR,
    CRITICAL
};

// Error handler class
// handles all exceptions
class ErrorHandler {
public:
    // this is like a variable for holding any function
    // that has error severity and string as parameter
    // and using is for giving this function a nickname
    // for cleaner more readable code
    using ErrorCallback = std::function<void(ErrorSeverity, const std::string&)>;

    // Singleton instance ensuring only one instance of error
    // for easy management
    static ErrorHandler& instance();

    // Thread safe Custom logging and stores that log in
    // errorCallback which is to be used in logError
    void setErrorCallback(ErrorCallback callback);

    // Template method to create a custom exception for logging the error message
    // and throw it
    // It is reusable
    template<typename TExceptionType = FileManagerException, typename... TArgs>
    [[noreturn]] static void raiseError(const TArgs&... args) {
        std::ostringstream oss;
        (oss << ... << args);       // fold expression for variable args
        std::string errorMessage = oss.str();

        instance().logError(ErrorSeverity::ERROR, errorMessage);
        throw TExceptionType(errorMessage);
    }

    // Assertion method for throwing if condition is false
    template<typename TExceptionType = FileManagerException, typename... TArgs>
    static void assert(bool condition, const TArgs&... args) {
        if (!condition) {
            raiseError<TExceptionType>(args...);
        }
    }

    // Standard error codes for non-throwing flows
    enum class ErrorCode {
        SUCCESS = 0,
        FILE_NOT_FOUND,
        PERMISSION_DENIED,
        INVALID_ARGUMENT,
        OUT_OF_MEMORY,
        PLUGIN_LOAD_FAILED,
        GUI_LOAD_FAILED,
        UNKNOWN_ERROR,
    };

    // Result wrapper for function that returns either value or error
    template<typename T>
    class Result {
    private:
        T value_;
        ErrorCode errorCode_;
        std::string errorMessage_;
        bool hasValue_;

    public:
        // Constructor when success
        Result(T&& value)
            : value_(std::move(value)), errorCode_(ErrorCode::SUCCESS), hasValue_(true) {}

        // Error constructor
        Result(ErrorCode code, const std::string& message)
            : errorCode_(code), errorMessage_(message), hasValue_(false) {}

        bool isSuccess() const { return hasValue_; }
        bool isFailure() const { return !hasValue_; }

        // Returns a reference to the stored value if successful
        // otherwise throws an exception
        const T& value() const {
            if (!hasValue_)
                raiseError<FileManagerException>("Attempted to access value from failed result");
            return value_;
        }

        // Same as value() but returns using std::move for efficiency
        T&& moveValue() {
            if (!hasValue_)
                raiseError<FileManagerException>("Attempted to access value from failed Result");
            return std::move(value_);
        }

        ErrorCode errorCode() const { return errorCode_; }
        const std::string& errorMessage() const { return errorMessage_; }
    };

    // Handling the Errors
    // and Mapping to appropriate exceptions
    static void handleSystemError(const std::string& operation);

    // Wrapper that runs any operation
    // returns a Result<T> with the output if successful
    // and Logs the errors automatically
    template<typename Func>
    static auto safeExecute(Func&& func, const std::string& operation) -> Result<decltype(func())> {
        try {
            return Result<decltype(func())>(func());
        }
        // handle known exceptions
        catch (const std::exception& e) {
            instance().logError(ErrorSeverity::ERROR, operation + " failed: " + e.what());
            return Result<decltype(func())>(ErrorCode::UNKNOWN_ERROR, e.what());
        }
        // handle unknown exceptions
        catch (...) {
            std::string msg = operation + " failed with unknown exception";
            instance().logError(ErrorSeverity::CRITICAL, msg);
            return Result<decltype(func())>(ErrorCode::UNKNOWN_ERROR, msg);
        }
    }

    // Logging a warning message (Non-Throwing)
    template<typename... TArgs>
    static void warning(const TArgs&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        instance().logError(ErrorSeverity::WARNING, oss.str());
    }

    // Critical failure (Terminates Application)
    template<typename... TArgs>
    [[noreturn]] static void critical(const TArgs&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        instance().logError(ErrorSeverity::CRITICAL, oss.str());
        std::terminate();  // Immediately stop the program
    }

    template<typename... Args>
    static std::string format(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);  // Fold expression in C++17
        return oss.str();
    }
    template<typename... Args>
    static void info(Args&&... args) {
        instance().logError(ErrorSeverity::WARNING, format(std::forward<Args>(args)...));
    }

    template<typename... Args>
   static void warning(Args&&... args) {
        instance().logError(ErrorSeverity::WARNING, format(std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void error(Args&&... args) {
        instance().logError(ErrorSeverity::ERROR, format(std::forward<Args>(args)...));
    }


    template<typename... Args>
    static void critical(Args&&... args) {
        critical(format(std::forward<Args>(args)...));
    }


private:
    mutable std::mutex mutex_;      // Thread safety
    ErrorCallback errorCallback_;   // Custom callback

    // Private constructor
    ErrorHandler();
    ~ErrorHandler();
    // for preventing copying
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    // Core method for logging errors
    void logError(ErrorSeverity severity, const std::string& message);


};



// Macros for more readable usage
#define FM_ASSERT(condition, ...) \
ErrorHandler::assert(condition, __VA_ARGS__)

#define FM_REQUIRE(condition, ...) \
ErrorHandler::assert<FileManagerException>(condition, "Requirement failed: ", __VA_ARGS__)

#define FM_THROW(...) \
ErrorHandler::raiseError<FileManagerException>(__VA_ARGS__)

#define FM_THROW_FS(...) \
ErrorHandler::raiseError<FileSystemException>(__VA_ARGS__)

#define FM_THROW_PLUGIN(...) \
ErrorHandler::raiseError<PluginException>(__VA_ARGS__)

#define FM_THROW_GUI(...) \
ErrorHandler::raiseError<GuiException>(__VA_ARGS__)

#define FM_WARNING(...) \
ErrorHandler::warning(__VA_ARGS__)

#define FM_CRITICAL(...) \
ErrorHandler::critical(__VA_ARGS__)

#define FM_ERROR(...) \
ErrorHandler::error(__VA_ARGS__)

#define FM_INFO(...) \
ErrorHandler::info(__VA_ARGS__)
