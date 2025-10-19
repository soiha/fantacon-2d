#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace Engine {

// Log levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

// Log output destination
class LogOutput {
public:
    virtual ~LogOutput() = default;
    virtual void write(const std::string& message) = 0;
    virtual void flush() = 0;
};

// Console output (default)
class ConsoleOutput : public LogOutput {
public:
    explicit ConsoleOutput(std::ostream& stream = std::cout) : stream_(stream) {}

    void write(const std::string& message) override {
        stream_ << message << std::endl;
    }

    void flush() override {
        stream_.flush();
    }

private:
    std::ostream& stream_;
};

// File output
class FileOutput : public LogOutput {
public:
    explicit FileOutput(const std::string& filename)
        : file_(filename, std::ios::out | std::ios::app) {
        if (!file_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }

    void write(const std::string& message) override {
        if (file_.is_open()) {
            file_ << message << std::endl;
        }
    }

    void flush() override {
        if (file_.is_open()) {
            file_.flush();
        }
    }

private:
    std::ofstream file_;
};

// Logger singleton
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Configure logger
    void setOutput(std::unique_ptr<LogOutput> output) {
        output_ = std::move(output);
    }

    void setMinLevel(LogLevel level) {
        minLevel_ = level;
    }

    void setShowTimestamp(bool show) { showTimestamp_ = show; }
    void setShowWallClock(bool show) { showWallClock_ = show; }
    void setShowFrame(bool show) { showFrame_ = show; }
    void setShowLevel(bool show) { showLevel_ = show; }

    // Frame tracking
    void setFrameNumber(uint64_t frame) { frameNumber_ = frame; }
    uint64_t getFrameNumber() const { return frameNumber_; }

    // Logging methods
    void log(LogLevel level, const std::string& message,
             const char* file = nullptr, int line = -1) {
        if (level < minLevel_) return;
        if (!output_) return;

        std::ostringstream oss;

        // Timestamp (time since logger creation)
        if (showTimestamp_) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - startTime_).count();
            oss << "[" << std::setw(8) << std::setfill(' ') << elapsed << "ms] ";
        }

        // Wall clock time
        if (showWallClock_) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            std::tm tm;
            #ifdef _WIN32
                localtime_s(&tm, &time);
            #else
                localtime_r(&time, &tm);
            #endif

            oss << "[" << std::put_time(&tm, "%H:%M:%S")
                << "." << std::setw(3) << std::setfill('0') << ms.count() << "] ";
        }

        // Frame number
        if (showFrame_) {
            oss << "[F:" << std::setw(6) << std::setfill(' ') << frameNumber_ << "] ";
        }

        // Log level
        if (showLevel_) {
            oss << "[" << levelToString(level) << "] ";
        }

        // File and line (if provided)
        if (file && line >= 0) {
            // Extract just filename from path
            const char* filename = file;
            const char* lastSlash = file;
            while (*lastSlash) {
                if (*lastSlash == '/' || *lastSlash == '\\') {
                    filename = lastSlash + 1;
                }
                lastSlash++;
            }
            oss << "[" << filename << ":" << line << "] ";
        }

        // Message
        oss << message;

        output_->write(oss.str());
    }

    void debug(const std::string& message, const char* file = nullptr, int line = -1) {
        log(LogLevel::Debug, message, file, line);
    }

    void info(const std::string& message, const char* file = nullptr, int line = -1) {
        log(LogLevel::Info, message, file, line);
    }

    void warning(const std::string& message, const char* file = nullptr, int line = -1) {
        log(LogLevel::Warning, message, file, line);
    }

    void error(const std::string& message, const char* file = nullptr, int line = -1) {
        log(LogLevel::Error, message, file, line);
    }

    void flush() {
        if (output_) {
            output_->flush();
        }
    }

private:
    Logger()
        : output_(std::make_unique<ConsoleOutput>())
        , minLevel_(LogLevel::Info)
        , showTimestamp_(true)
        , showWallClock_(true)
        , showFrame_(true)
        , showLevel_(true)
        , frameNumber_(0)
        , startTime_(std::chrono::steady_clock::now()) {
    }

    const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO ";
            case LogLevel::Warning: return "WARN ";
            case LogLevel::Error:   return "ERROR";
        }
        return "UNKNOWN";
    }

    std::unique_ptr<LogOutput> output_;
    LogLevel minLevel_;
    bool showTimestamp_;
    bool showWallClock_;
    bool showFrame_;
    bool showLevel_;
    uint64_t frameNumber_;
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace Engine

// Convenient logging macros (use ::Engine::Logger for global scope)
#define LOG_DEBUG(msg) \
    ::Engine::Logger::getInstance().debug(msg, __FILE__, __LINE__)

#define LOG_INFO(msg) \
    ::Engine::Logger::getInstance().info(msg, __FILE__, __LINE__)

#define LOG_WARNING(msg) \
    ::Engine::Logger::getInstance().warning(msg, __FILE__, __LINE__)

#define LOG_ERROR(msg) \
    ::Engine::Logger::getInstance().error(msg, __FILE__, __LINE__)

// Stream-style macros for easier formatting
#define LOG_DEBUG_STREAM(stream) \
    do { \
        std::ostringstream oss; \
        oss << stream; \
        ::Engine::Logger::getInstance().debug(oss.str(), __FILE__, __LINE__); \
    } while(0)

#define LOG_INFO_STREAM(stream) \
    do { \
        std::ostringstream oss; \
        oss << stream; \
        ::Engine::Logger::getInstance().info(oss.str(), __FILE__, __LINE__); \
    } while(0)

#define LOG_WARNING_STREAM(stream) \
    do { \
        std::ostringstream oss; \
        oss << stream; \
        ::Engine::Logger::getInstance().warning(oss.str(), __FILE__, __LINE__); \
    } while(0)

#define LOG_ERROR_STREAM(stream) \
    do { \
        std::ostringstream oss; \
        oss << stream; \
        ::Engine::Logger::getInstance().error(oss.str(), __FILE__, __LINE__); \
    } while(0)

// Printf-style macros for easier formatting with format strings
#define LOG_DEBUG_FMT(fmt, ...) \
    do { \
        char buffer[1024]; \
        std::snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        ::Engine::Logger::getInstance().debug(buffer, __FILE__, __LINE__); \
    } while(0)

#define LOG_INFO_FMT(fmt, ...) \
    do { \
        char buffer[1024]; \
        std::snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        ::Engine::Logger::getInstance().info(buffer, __FILE__, __LINE__); \
    } while(0)

#define LOG_WARNING_FMT(fmt, ...) \
    do { \
        char buffer[1024]; \
        std::snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        ::Engine::Logger::getInstance().warning(buffer, __FILE__, __LINE__); \
    } while(0)

#define LOG_ERROR_FMT(fmt, ...) \
    do { \
        char buffer[1024]; \
        std::snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        ::Engine::Logger::getInstance().error(buffer, __FILE__, __LINE__); \
    } while(0)
