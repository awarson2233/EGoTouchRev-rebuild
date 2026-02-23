/**
 * @file Logger.cpp
 * @brief 通用日志模块实现
 */
#include "Logger.h"
#include <iostream>

namespace Common {

std::shared_ptr<spdlog::logger> Logger::s_logger = nullptr;

void Logger::Init(const std::string& loggerName, const std::filesystem::path& logDir) {
    if (s_logger != nullptr) {
        return; // Already initialized
    }

    try {
        namespace fs = std::filesystem;
        
        // 创建日志目录
        std::error_code ec;
        fs::create_directories(logDir, ec);
        if (ec) {
            std::cerr << "Failed to create log directory: " << ec.message() << std::endl;
            return;
        }

        // 初始化异步日志线程池
        spdlog::init_thread_pool(8192, 1);

        // 1. Console Sink (带颜色)
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // 格式: [%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-7l%$] %v");

        // 2. Daily File Sink (每天午夜轮转，最多保留 7 天)
        fs::path file_path = logDir / (loggerName + ".txt");
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(file_path.string(), 0, 0, false, 7);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-7l] %v");

        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};

        // 创建异步 Logger
        s_logger = std::make_shared<spdlog::async_logger>(
            loggerName, 
            sinks.begin(), sinks.end(), 
            spdlog::thread_pool(), 
            spdlog::async_overflow_policy::block
        );

        s_logger->set_level(spdlog::level::trace);
        s_logger->flush_on(spdlog::level::warn);

        spdlog::register_logger(s_logger);
        spdlog::set_default_logger(s_logger);

        // 分隔线，区分每次启动
        s_logger->info("========================================");
        s_logger->info("Logger Initialized: {}", loggerName);

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void Logger::Shutdown() {
    if (s_logger) {
        s_logger->info("Logger Shutting down.");
        spdlog::shutdown();
        s_logger.reset();
    }
}

} // namespace Common
