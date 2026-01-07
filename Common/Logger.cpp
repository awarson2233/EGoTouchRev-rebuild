/**
 * @file Logger.cpp
 * @brief 通用日志模块实现
 */
#include "Logger.h"
#include <chrono>
#include <format>
#include <iostream>

namespace Common {

Logger::Logger(const std::string& name, const std::filesystem::path& logDir)
    : m_name(name), m_logDir(logDir) {
    InitLogFile();
}

Logger::~Logger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void Logger::InitLogFile() {
    namespace fs = std::filesystem;
    using namespace std::chrono;

    // 获取今天的日期
    const auto today = floor<days>(system_clock::now());
    const std::string date_str = std::format("{:%Y%m%d}", today);

    // 创建日志目录
    std::error_code ec;
    fs::create_directories(m_logDir, ec);
    if (ec) {
        std::cerr << "Failed to create log directory: " << ec.message() << std::endl;
        return;
    }

    // 生成日志文件路径: {logDir}/{name}_log_{date}
    const fs::path file_path = m_logDir / std::format("{}_log_{}", m_name, date_str);
    const bool existed = fs::exists(file_path);

    // 以追加模式打开
    m_file.open(file_path, std::ios::out | std::ios::app);
    if (m_file.is_open() && existed) {
        // 如果文件已存在，添加分隔符
        const std::string sep(40, '-');
        m_file << sep << '\n'
               << '\n' << '\n' << '\n' << '\n' << '\n'
               << sep << '\n';
        m_file.flush();
    }
}

std::string Logger::Log(const std::string& message, const char* func) {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto sec_tp = floor<seconds>(now);
    const auto usec = duration_cast<microseconds>(now - sec_tp).count();

    const std::string line = std::format(
        "{} [{:%H:%M:%S}:{:06d}] [{}] {}",
        m_name,
        sec_tp,
        static_cast<int>(usec),
        func ? func : "-",
        message);

    // 输出到控制台
    std::cout << line << std::endl;

    // 输出到文件
    if (m_file.is_open()) {
        m_file << line << std::endl;
    }

    return line;
}

} // namespace Common
