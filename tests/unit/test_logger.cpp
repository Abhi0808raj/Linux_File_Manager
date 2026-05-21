#include <gtest/gtest.h>
// <gtest/gtest.h> transitively includes <cassert>, which defines assert() as
// a C macro. That macro conflicts with the ErrorHandler::assert() static
// template method declared in error_handler.hpp (included via logger.hpp).
// Undefine it here so the header parses correctly.
#ifdef assert
#  undef assert
#endif
#include "utilities/logger.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

class LoggerTest : public ::testing::Test {
protected:
    fs::path logPath;

    void SetUp() override {
        logPath = fs::temp_directory_path() / "fm_unit_logger.log";
        fs::remove(logPath);
    }

    void TearDown() override {
        fs::remove(logPath);
    }

    std::string readLog() {
        std::ifstream f(logPath);
        return {std::istreambuf_iterator<char>(f), {}};
    }
};

// --- Severity labels ---

TEST_F(LoggerTest, InfoLabelAppearsInLog) {
    { Logger logger(logPath.string()); logger.log(ErrorSeverity::INFO, "info_msg"); }
    EXPECT_NE(readLog().find("[INFO]"), std::string::npos);
    EXPECT_NE(readLog().find("info_msg"), std::string::npos);
}

TEST_F(LoggerTest, WarningLabelAppearsInLog) {
    { Logger logger(logPath.string()); logger.log(ErrorSeverity::WARNING, "warn_msg"); }
    EXPECT_NE(readLog().find("[WARNING]"), std::string::npos);
}

TEST_F(LoggerTest, ErrorLabelAppearsInLog) {
    { Logger logger(logPath.string()); logger.log(ErrorSeverity::ERROR, "err_msg"); }
    EXPECT_NE(readLog().find("[ERROR]"), std::string::npos);
}

TEST_F(LoggerTest, CriticalLabelAppearsInLog) {
    { Logger logger(logPath.string()); logger.log(ErrorSeverity::CRITICAL, "crit_msg"); }
    EXPECT_NE(readLog().find("[CRITICAL]"), std::string::npos);
}

// --- Timestamp ---

TEST_F(LoggerTest, TimestampFormattedAsYYYYMMDD) {
    { Logger logger(logPath.string()); logger.log(ErrorSeverity::INFO, "ts_test"); }
    // Log format: [YYYY-MM-DD HH:MM:SS]
    EXPECT_NE(readLog().find("[20"), std::string::npos);
}

// --- Append mode ---

TEST_F(LoggerTest, SecondLoggerInstanceAppendsToExistingContent) {
    { Logger l1(logPath.string()); l1.log(ErrorSeverity::INFO, "first_write"); }
    { Logger l2(logPath.string()); l2.log(ErrorSeverity::INFO, "second_write"); }
    std::string content = readLog();
    EXPECT_NE(content.find("first_write"), std::string::npos);
    EXPECT_NE(content.find("second_write"), std::string::npos);
}

// --- Thread safety ---

TEST_F(LoggerTest, ConcurrentWritesAllMessagesReachLog) {
    const int kThreads = 10;
    const int kMsgsPerThread = 20;

    {
        Logger logger(logPath.string());
        std::vector<std::thread> threads;
        for (int i = 0; i < kThreads; ++i) {
            threads.emplace_back([&, i]() {
                for (int j = 0; j < kMsgsPerThread; ++j) {
                    logger.log(ErrorSeverity::INFO,
                        "t" + std::to_string(i) + "_m" + std::to_string(j));
                }
            });
        }
        for (auto& t : threads) t.join();
    }  // logger destroyed here — file closed before counting

    std::string content = readLog();
    int count = 0;
    size_t pos = 0;
    while ((pos = content.find("[INFO]", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    EXPECT_EQ(count, kThreads * kMsgsPerThread);
}
