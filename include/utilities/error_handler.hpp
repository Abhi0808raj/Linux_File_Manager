#pragma once

#include<exception>
#include<string>
#include<sstream>        //for string based stream operations
#include<iostream>
#include<mutex>          //for thread safe operations
#include<functional>     //used to store custom error callback function
#include<cstring>        //provides C style string manipulation
#include<cerrno>        //for identifying cause of low level system errors

//Base Class for custom Exception

class FileManagerException : public std::exception {
protected:
    std::string m_message; //holds error message
public:
    //Copy constructor that takes string by reference. Used to pass message in a string
    FileManagerException(const std::string& message) : m_message(m_message) {}

    //Move constructor for handling temporary strings for efficiency
    FileManagerException(std::string&& message) : m_message(std::move(message)) {}

    //tells the copiler to not throw exception and and return message
    const char* what() const noexcept override {
    return m_message.c_str();
    }
    const std::string& message() const noexcept {
    return m_message;
    }
};

// Derived Exception types for specific modules
class FileSystemException : public FileManagerException {
public:
    FileSystemException(const std::string& message)
        : FileManagerException("FileSystem Error: " + message) {}
};

class PluginException : public FileManagerException {
public:
    PluginException(const std::string& message)
        : FileManagerException("Plugin Error: " + message) {}
};

class GuiException : public FileManagerException {
public:
    GuiException(const std::string& message)
        : FileManagerException("GUI Error: " + message) {}
};

// class ErrorException : public FileManagerException
// {
// public:
//     ErrorException(const std::string& message)
//     : FileManagerException("Error: " + message) {}
// };

//Enum for categorizing error severity

enum class ErrorSeverity {
    WARNING,
    ERROR,
    CRITICAL
};

//Error handler class
//handles all exceptions

class ErrorHandler {
    public:
    //this is like a variable for holding any function
    //that as error severity and string as parameter
    //and using is for giving this function a nickname
    //for cleaner more readable code
    using ErrorCallback = std::function<void(ErrorSeverity , const std::string& )>;

    //Singleton instance ensuring only one instance of error
    //for easy management
    static ErrorHandler& instance() {
        static ErrorHandler instance;
        return instance;
    }

    //Thread safe Custom logging and stores that log in
    //errorCallback which is to be used in logError
    void setErrorCallback(ErrorCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        errorCallback_ = callback;
    }

    //Template method to create a custom exception for logging the error message
    //and throw is
    //It is resuable

    template<typename TExceptionType=FileManagerException,typename... TArgs>
    [[noreturn]] static void raiseError(const TArgs&... args) {
        std::ostringstream oss;
        (oss << ... << args);       //fold expression for variable args
        std::string errorMessage = oss.str();

        instance().logError(ErrorSeverity::ERROR,errorMessage);
        throw TExceptionType(errorMessage);
    }

    //Assertion method for throwing if condition is false
    template<typename TExceptionType=FileManagerException,typename... TArgs>
    static void assert(bool condition,const TArgs&... args) {
        if(!condition) {
            raiseError<TExceptionType>(args...);
        }
    }

    //Standard error codes for non-throwing flows

    enum class ErrorCode {
        SUCCESS=0,
        FILE_NOT_FOUND,
        PERMISSION_DENIED,
        INVALID_ARGUMENT,
        OUT_OF_MEMORY,
        PLUGIN_LOAD_FAILED,
        GUI_LOAD_FAILED,
        UNKNOWN_ERROR,
    };

    //Result wrapper for function that return either value or eroor

    template<typename T>
    class Result {
    private:
        T_value_;
        ErrorCode errorCode_;
        std::string errorMessage_;
        bool hasValue_;

    public:
        //Constructor when success
        Result(T&& value)
            :value_(std::move(value)),error_Code_(ErrorCode::SUCCESS),hasValue_(true) {}

        //Error constructor
        Result(ErrorCode code,const std::string& message)
            :errorCode_(code),errorMessage_(message),hasValue_(false) {}

        bool isSuccess() const {return hasValue_;}
        bool isFailure() const {return !hasValue_;}


        //Returns a reference to the stored value if successful
        //otherwise throws an exception
        const T& value() const {
            if(!hasValue_)
                raiseError<FileManagerException>("Attempted to access value from failed result");
            return value_;
        }

        //save as value() but returns using std::move for efficiency
        T&& moveValue() {
            if (!hasValue_)
                raiseError<FileManagerException>("Attempted to access value from failed Result");
            return std::move(value_);
        }
        ErrorCode errorCode() const { return errorCode_; }
        const std::string& errorMessage() const { return errorMessage_; }
    };

    //Handling the Errors
    //and Mapping to appropriate exceptions
    static void handleSystemError(const std::string& operation) {
        std::string errorMsg = operation + " failed: " + std::strerror(errno);
        instance().logError(ErrorSeverity::ERROR, errorMsg);


        //these errno come from library itself so no need to specify manually
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

    //Wrapper to that runs any operation
    //returns a Result<T> with the output if successful
    //and Logs the errors automatically
    template<typename Func>
    static auto safeExecute(Func&& func, const std::string& operation) -> Result<decltype(func())> {
        try {
            return Result<decltype(func())>(func());
        }
        //handle known exceptions
        catch (const std::exception& e) {
            instance().logError(ErrorSeverity::ERROR, operation + " failed: " + e.what());
            return Result<decltype(func())>(ErrorCode::UNKNOWN_ERROR, e.what());
        }
        //handle unknown exceptions
        catch (...) {
            std::string msg = operation + " failed with unknown exception";
            instance().logError(ErrorSeverity::CRITICAL, msg);
            return Result<decltype(func())>(ErrorCode::UNKNOWN_ERROR, msg);
        }
    }

    //Logging a warning  message ( Non-Throwing)
    template<typename... TArgs>
    static void warning(const TArgs&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        instance().logError(ErrorSeverity::WARNING, oss.str());
    }

    //Critical failure(Terminates Application)
    template<typename... TArgs>
    [[noreturn]] static void critical(const TArgs&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        instance().logError(ErrorSeverity::CRITICAL, oss.str());
        std::terminate();  // Immediately stop the program
    }

private:
    mutable std::mutex mutex_;      // Thread safety
    ErrorCallback errorCallback_;   // Custom callback

    // Private constructor
    ErrorHandler() = default;
    ~ErrorHandler() = default;
    //for preventing copying
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    //Core method for logging errors
    void logError(ErrorSeverity severity, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        const char* severityStr="";
        switch (severity) {
            case ErrorSeverity::WARNING: severityStr="WARNING"; break;
            case ErrorSeverity::ERROR: severityStr="ERROR"; break;
            case ErrorSeverity::CRITICAL: severityStr="CRITICAL"; break;
        }
        //Default logging to stderr
        std::cerr << "["<< severityStr<< "]" << message << std::endl;

        //If custom handler is there use it
        if (errorCallback_) {
            errorCallback_(severity,message);
        }
    }
};


//Macros for more readable usage

//check if condition is true and throws if false
#define FM_ASSERT(condition, ...) \
ErrorHandler::assert(condition, __VA_ARGS__)

//Requirement failed macro
#define FM_REQUIRE(condition, ...) \
ErrorHandler::assert<FileManagerException>(condition, "Requirement failed: ", __VA_ARGS__)

//Macro for throwing the message
#define FM_THROW(...) \
ErrorHandler::raiseError<FileManagerException>(__VA_ARGS__)

//Macro for file system exception
#define FM_THROW_FS(...) \
ErrorHandler::raiseError<FileSystemException>(__VA_ARGS__)

// Macro for throwing plugin exception
#define FM_THROW_PLUGIN(...) \
ErrorHandler::raiseError<PluginException>(__VA_ARGS__)

//Macro for throwing GUI Exception
#define FM_THROW_GUI(...) \
ErrorHandler::raiseError<GuiException>(__VA_ARGS__)

//Macro for logging warnings
#define FM_WARNING(...) \
ErrorHandler::warning(__VA_ARGS__)

//Macro for logging Critical errors and terminating the program
#define FM_CRITICAL(...) \#define FM_ASSERT(condition, ...) \
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
ErrorHandler::critical(__VA_ARGS__)
