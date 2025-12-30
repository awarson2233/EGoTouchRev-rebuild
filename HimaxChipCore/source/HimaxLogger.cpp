#include "HimaxLogger.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <format>

namespace Himax {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Init(const std::string& directory) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    namespace fs = std::filesystem;
    using namespace std::chrono;

    const auto today = floor<days>(system_clock::now());
    const std::string date_str = std::format("{:%Y%m%d}", today);

    std::error_code ec;
    // directory is passed in, e.g. "C:/ProgramData/EGoTouchRev/"
    // In Linux sandbox environment we might want to adjust this or keep it if it's just for compilation check.
    // The original code used a hardcoded Windows path. I will respect the structure but handle path creation.
    fs::path dir_path{directory};

    // Create directories if they don't exist
    if (!fs::exists(dir_path)) {
        fs::create_directories(dir_path, ec);
    }

    const fs::path file_path = dir_path / std::format("hx_hal_log_{}", date_str);
    const bool existed = fs::exists(file_path);

    m_logFile.open(file_path, std::ios::out | std::ios::app);

    if (m_logFile.is_open()) {
        if (existed) {
            const std::string sep(40, '-');
            m_logFile << sep << '\n'
                      << '\n' << '\n' << '\n' << '\n' << '\n'
                      << sep << '\n';
            m_logFile.flush();
        }
        m_initialized = true;
    }
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::Log(const std::string& message, const char* func) {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto sec_tp = floor<seconds>(now);
    const auto usec = duration_cast<microseconds>(now - sec_tp).count();

    const std::string line = std::format(
        "HIMAX [{:%H:%M:%S}:{:06d}] [{}] {}",
        sec_tp,
        static_cast<int>(usec),
        func ? func : "-",
        message);

    // Always output to stdout
    std::cout << line << std::endl;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile << line << std::endl;
    }
}

std::string LogWithTimestamp(const std::string& message, const char* func) {
    Logger::Instance().Log(message, func);
    return message; // Return message to keep compatible with existing usage if needed, though original returned formatted line
}

} // namespace Himax
