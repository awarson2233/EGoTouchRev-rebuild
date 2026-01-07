/**
 * @file Logger.h
 * @brief 通用日志模块
 * @description 提供带时间戳的日志记录功能，支持多实例独立日志文件
 */
#pragma once
#include <filesystem>
#include <fstream>
#include <string>

namespace Common {

/**
 * @class Logger
 * @brief 日志记录器，每个实例可输出到独立的日志文件
 */
class Logger {
public:
    /**
     * @brief 构造函数
     * @param name 日志器名称，用于标识日志来源和生成文件名
     * @param logDir 日志存放目录（默认: C:/ProgramData/EGoTouchRev/）
     */
    explicit Logger(const std::string& name, 
                    const std::filesystem::path& logDir = "C:/ProgramData/EGoTouchRev/");
    
    ~Logger();

    /**
     * @brief 记录日志消息
     * @param message 日志内容
     * @param func 调用者函数名（可选，通常通过 LOG 宏自动传入）
     * @return 格式化后的日志行
     */
    std::string Log(const std::string& message, const char* func = nullptr);

    /**
     * @brief 检查日志文件是否成功打开
     */
    bool IsOpen() const { return m_file.is_open(); }

    /**
     * @brief 获取日志器名称
     */
    const std::string& GetName() const { return m_name; }

private:
    std::string m_name;
    std::ofstream m_file;
    std::filesystem::path m_logDir;
    
    void InitLogFile();
};

/**
 * @brief 便捷日志宏，自动附加函数名
 * @param logger Logger 实例引用
 * @param msg 日志消息
 */
#define COMMON_LOG(logger, msg) (logger).Log((msg), __func__)

} // namespace Common
