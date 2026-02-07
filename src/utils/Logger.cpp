#include "utils/Logger.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

/**
 * @file Logger.cpp
 * @brief Simple logging utility implementation
 */

// Static member initialization
LogLevel Logger::currentLevel_ = LogLevel::INFO;
std::mutex Logger::logMutex_;

std::string Logger::getCurrentTimestamp()
{
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::levelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void Logger::setLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(logMutex_);
    currentLevel_ = level;
}

void Logger::log(LogLevel level, const std::string &message)
{
    if (level < currentLevel_)
    {
        return; // Skip messages below current log level
    }

    std::lock_guard<std::mutex> lock(logMutex_);

    std::ostream &output = (level == LogLevel::ERROR) ? std::cerr : std::cout;

    output << "[" << getCurrentTimestamp() << "] "
           << "[" << levelToString(level) << "] " << message << std::endl;
}

void Logger::debug(const std::string &message)
{
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string &message)
{
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string &message)
{
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string &message)
{
    log(LogLevel::ERROR, message);
}
