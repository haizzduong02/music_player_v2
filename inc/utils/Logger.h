#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <iostream>

/**
 * @file Logger.h
 * @brief Singleton logger utility
 * 
 * Provides thread-safe logging functionality for the application.
 */

/**
 * @brief Log level enumeration
 */
enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

/**
 * @brief Logger singleton class
 * 
 * Thread-safe logging utility using Singleton pattern.
 */
class Logger {
public:
    /**
     * @brief Get the logger instance
     * @return Reference to the singleton logger
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief Initialize logger with log file path
     * @param filepath Path to log file
     */
    void init(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFile_.open(filepath, std::ios::app);
        logFilePath_ = filepath;
    }
    
    /**
     * @brief Log an info message
     * @param message Message to log
     */
    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }
    
    /**
     * @brief Log a warning message
     * @param message Message to log
     */
    void warn(const std::string& message) {
        log(LogLevel::WARNING, message);
    }
    
    /**
     * @brief Log an error message
     * @param message Message to log
     */
    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }
    
    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }
    
    /**
     * @brief Set minimum log level
     * @param level Minimum level to log
     */
    void setLogLevel(LogLevel level) {
        minLevel_ = level;
    }
    
private:
    Logger() : minLevel_(LogLevel::INFO) {}
    
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }
    
    /**
     * @brief Core log function
     * @param level Log level
     * @param message Message to log
     */
    void log(LogLevel level, const std::string& message) {
        if (level < minLevel_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string levelStr;
        switch (level) {
            case LogLevel::INFO:    levelStr = "[INFO]    "; break;
            case LogLevel::WARNING: levelStr = "[WARNING] "; break;
            case LogLevel::ERROR:   levelStr = "[ERROR]   "; break;
            case LogLevel::DEBUG:   levelStr = "[DEBUG]   "; break;
        }
        
        // Get current time
        time_t now = time(nullptr);
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        std::string logEntry = std::string(timeStr) + " " + levelStr + message + "\n";
        
        // Write to file
        if (logFile_.is_open()) {
            logFile_ << logEntry;
            logFile_.flush();
        }
        
        // Also write to console
        std::cout << logEntry;
    }
    
    std::ofstream logFile_;
    std::string logFilePath_;
    std::mutex mutex_;
    LogLevel minLevel_;
};

#endif // LOGGER_H
