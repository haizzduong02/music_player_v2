#include "utils/Logger.h"
#include <gtest/gtest.h>
#include <sstream>
#include <thread>
#include <vector>

class LoggerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Reset to default log level for each test
        Logger::setLogLevel(LogLevel::DEBUG);
    }
};

// Log level setting tests
TEST_F(LoggerTest, SetLogLevelDebug)
{
    Logger::setLogLevel(LogLevel::DEBUG);
    // Should not throw
    EXPECT_NO_THROW(Logger::debug("Debug message"));
    EXPECT_NO_THROW(Logger::info("Info message"));
    EXPECT_NO_THROW(Logger::warn("Warn message"));
    EXPECT_NO_THROW(Logger::error("Error message"));
}

TEST_F(LoggerTest, SetLogLevelInfo)
{
    Logger::setLogLevel(LogLevel::INFO);
    EXPECT_NO_THROW(Logger::debug("Debug message")); // Should be filtered but not throw
    EXPECT_NO_THROW(Logger::info("Info message"));
    EXPECT_NO_THROW(Logger::warn("Warn message"));
    EXPECT_NO_THROW(Logger::error("Error message"));
}

TEST_F(LoggerTest, SetLogLevelWarn)
{
    Logger::setLogLevel(LogLevel::WARN);
    EXPECT_NO_THROW(Logger::debug("Debug message"));
    EXPECT_NO_THROW(Logger::info("Info message"));
    EXPECT_NO_THROW(Logger::warn("Warn message"));
    EXPECT_NO_THROW(Logger::error("Error message"));
}

TEST_F(LoggerTest, SetLogLevelError)
{
    Logger::setLogLevel(LogLevel::ERROR);
    EXPECT_NO_THROW(Logger::debug("Debug message"));
    EXPECT_NO_THROW(Logger::info("Info message"));
    EXPECT_NO_THROW(Logger::warn("Warn message"));
    EXPECT_NO_THROW(Logger::error("Error message"));
}

// All log methods tests
TEST_F(LoggerTest, DebugLogging)
{
    EXPECT_NO_THROW(Logger::debug("This is a debug message"));
}

TEST_F(LoggerTest, InfoLogging)
{
    EXPECT_NO_THROW(Logger::info("This is an info message"));
}

TEST_F(LoggerTest, WarnLogging)
{
    EXPECT_NO_THROW(Logger::warn("This is a warning message"));
}

TEST_F(LoggerTest, ErrorLogging)
{
    EXPECT_NO_THROW(Logger::error("This is an error message"));
}

// Empty message tests
TEST_F(LoggerTest, EmptyMessages)
{
    EXPECT_NO_THROW(Logger::debug(""));
    EXPECT_NO_THROW(Logger::info(""));
    EXPECT_NO_THROW(Logger::warn(""));
    EXPECT_NO_THROW(Logger::error(""));
}

// Long message tests
TEST_F(LoggerTest, LongMessages)
{
    std::string longMessage(1000, 'a');
    EXPECT_NO_THROW(Logger::debug(longMessage));
    EXPECT_NO_THROW(Logger::info(longMessage));
}

// Special character tests
TEST_F(LoggerTest, SpecialCharacters)
{
    EXPECT_NO_THROW(Logger::info("Message with special chars: \t\n\r"));
    EXPECT_NO_THROW(Logger::info("Unicode: 日本語テスト"));
    EXPECT_NO_THROW(Logger::info("Emoji: 🎵🎶"));
}

// Thread safety test
TEST_F(LoggerTest, ThreadSafety)
{
    const int numThreads = 10;
    const int messagesPerThread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; ++j)
            {
                Logger::info("Thread " + std::to_string(i) + " message " + std::to_string(j));
            }
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // If we get here without crashes/deadlocks, the test passes
    SUCCEED();
}

// Multiple consecutive logs
TEST_F(LoggerTest, ConsecutiveLogs)
{
    for (int i = 0; i < 100; ++i)
    {
        Logger::debug("Debug " + std::to_string(i));
        Logger::info("Info " + std::to_string(i));
        Logger::warn("Warn " + std::to_string(i));
        Logger::error("Error " + std::to_string(i));
    }
    SUCCEED();
}
