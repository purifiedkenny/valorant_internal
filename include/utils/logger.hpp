#pragma once

#include <string>
#include <memory>
#include <format>

namespace maxi::utils {

    


    enum class LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR_LEVEL,
        FATAL
    };

    


    class ILogger {
    public:
        virtual ~ILogger() = default;

        virtual void log(LogLevel level, const std::string& message) = 0;
        virtual void log(LogLevel level, const std::wstring& message) = 0;


        virtual void trace(const std::string& message) = 0;
        virtual void debug(const std::string& message) = 0;
        virtual void info(const std::string& message) = 0;
        virtual void warning(const std::string& message) = 0;
        virtual void error(const std::string& message) = 0;
        virtual void fatal(const std::string& message) = 0;


        virtual void set_level(LogLevel level) = 0;
        virtual LogLevel get_level() const = 0;
    };

    


    class LoggerFactory {
    public:
        static std::shared_ptr<ILogger> create_console_logger(LogLevel level = LogLevel::INFO);
        static std::shared_ptr<ILogger> create_file_logger(const std::string& filename, LogLevel level = LogLevel::INFO);
        static std::shared_ptr<ILogger> create_null_logger();
    };

    extern std::shared_ptr<ILogger> g_logger;

    void initialize_logger(LogLevel level = LogLevel::DEBUG);
    void set_global_log_level(LogLevel level);


    #define LOG_TRACE(msg) if (maxi::utils::g_logger) maxi::utils::g_logger->trace(msg)
    #define LOG_DEBUG(msg) if (maxi::utils::g_logger) maxi::utils::g_logger->debug(msg)
    #define LOG_INFO(msg) if (maxi::utils::g_logger) maxi::utils::g_logger->info(msg)
    #define LOG_WARNING(msg) if (maxi::utils::g_logger) maxi::utils::g_logger->warning(msg)
    #define LOG_ERROR(msg) if (maxi::utils::g_logger) maxi::utils::g_logger->error(msg)
    #define LOG_FATAL(msg) if (maxi::utils::g_logger) maxi::utils::g_logger->fatal(msg)

}
