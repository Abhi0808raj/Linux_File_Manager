#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>

#include "error_handler.hpp" // for using ErrorSeverity enum

// Thread-safe Logger class
// Used to log error messages to file (and optionally to console)
class Logger {
public:
  // Constructor that opens the log file
  // The file is opened in append mode so previous logs are preserved
  Logger(const std::string& filename = "file_manager.log");

  // Destructor that ensures file is properly closed
  ~Logger();

  // Logs the error message with given severity (WARNING, ERROR, CRITICAL)
  // Automatically prepends timestamp and severity label
  void log(ErrorSeverity severity, const std::string& message);

private:
  std::ofstream logFile;   // Output file stream to store logs
  std::mutex mutex_;       // Mutex for ensuring thread-safe logging
};
