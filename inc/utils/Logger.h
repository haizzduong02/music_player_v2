#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>

/**
 * @file Logger.h
 * @brief Static logger utility
 * 
 * Provides thread-safe logging functionality for the application.
 * Uses static methods - no instance needed.
 */

/**
 * @brief Log level enumeration
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

/**
 * @brief Static Logger class
 * 
 * Thread-safe logging utility using static methods.
 */
class Logger {
public:
    // Delete constructors - static only
    Logger() = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief Set minimum log level
     * @param level Minimum level to log
     */
    static void setLogLevel(LogLevel level);
    
    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    static void debug(const std::string& message);
    
    /**
     * @brief Log an info message
     * @param message Message to log
     */
    static void info(const std::string& message);
    
    /**
     * @brief Log a warning message
     * @param message Message to log
     */
    static void warn(const std::string& message);
    
    /**
     * @brief Log an error message
     * @param message Message to log
     */
    static void error(const std::string& message);
    
private:
    static LogLevel currentLevel_;
    static std::mutex logMutex_;
    
    /**
     * @brief Core log function
     * @param level Log level
     * @param message Message to log
     */
    static void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Get current timestamp string
     * @return Formatted timestamp
     */
    static std::string getCurrentTimestamp();
    
    /**
     * @brief Convert log level to string
     * @param level Log level
     * @return String representation
     */
    static std::string levelToString(LogLevel level);
};

#endif // LOGGER_H
