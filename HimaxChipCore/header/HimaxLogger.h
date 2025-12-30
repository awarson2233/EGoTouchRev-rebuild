#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace Himax {

class Logger {
public:
    static Logger& Instance();

    void Init(const std::string& directory);
    void Log(const std::string& message, const char* func = nullptr);

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;
    ~Logger();

    std::ofstream m_logFile;
    std::mutex m_mutex;
    bool m_initialized = false;
};

// Helper function to match existing usage pattern
std::string LogWithTimestamp(const std::string& message, const char* func = nullptr);

} // namespace Himax

#define HIMAX_LOG(msg) Himax::LogWithTimestamp((msg), __func__)
