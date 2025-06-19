#pragma once

#include<iostream>  //for cout
#include<fstream>   //for writing to files
#include<mutex>     //for mutex(thread safety)
#include<string>    //for strings
#include<chrono>    //for getting system time
#include<ctime>     //for formatting time

class Logger{
  public:
    //Enum to define different logging levels
    enum class Level{
      INFO,
      WARNING,
      ERROR
    };

    //Get the singleton instance of the logger
    static Logger& instance(){
      static Logger logger_instance;  //thread-safe initialization
      return logger_instance;
    }

    void setLogFile(const std::string& filename){
      std::lock_guard<std::mutex> lock(mutex_);   //Lock for thread safety
      if(logfile_.is_open()){
        logfile_.close();     //close any existing file
      }
      logfile_.open(filename,std::ios::app); //open in append mode 
    }

    //Set the output log file; any existing open file is closed
    void log(Level level, const std::string& message){
      std::lock_guard<std::mutex> lock(mutex_);   //Lock for thread safety
      
      std::string level_str=levelToString(level); //convert level to string
      std:: string timestamp=currentDateTime();   //get current time
      
      std::string log_entry="["+timestamp+"] ["+level_str+"]" +message+"\n";

      if(logfile_.is_open()){
        logfile_<<log_entry;
        logfile_.flush();   //write immediately
      }
      else{
        std::cout<<log_entry;
      }
    }

    //Convenience method for info messages
    void info(const std::string& message){
      log(Level::INFO,message);
    }

    // Convenience method for warning messages
    void warning(const std::string& message) {
        log(Level::WARNING, message);
    }

    // Convenience method for error messages
    void error(const std::string& message) {
        log(Level::ERROR, message);
    }

  private:
    std::ofstream logfile_;   //Output file stream for the log
    std::mutex mutex_;        //Mutex to ensure thread safety

    Logger()=default;         //Private constructor to ensure only one instance is these
    ~Logger(){                //Destructor to to the file properly
      if(logfile_.is_open()){
        logfile_.close();
      }
    }

    //Deleting the logger instance so that if someone creates a copy, it throws error
    Logger(const Logger&)=delete;
    Logger& operator=(const Logger&)=delete;


    //Converting log level enum to string
    std::string levelToString(Level level) {
        switch (level) {
            case Level::INFO: return "INFO";
            case Level::WARNING: return "WARNING";
            case Level::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    // Return the current system time formatted as a string
    std::string currentDateTime() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        char buf[20];
        //Setting String time format
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
        return std::string(buf);
    }
};


