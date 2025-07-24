#include "log.h"
#include <iostream>

namespace rediscc {
namespace logger {

bool InitLogger(const std::string& config_file) {
    try {
        return LoggerManager::Instance().Initialize(config_file);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return false;
    }
}

void ShutdownLogger() {
    try {
        LoggerManager::Instance().Shutdown();
    } catch (const std::exception& e) {
        std::cerr << "Error during logger shutdown: " << e.what() << std::endl;
    }
}

void SetLogContext(const LogContext& context) {
    LoggerManager::Instance().SetContext(context);
}

void ClearLogContext() {
    LoggerManager::Instance().ClearContext();
}

LoggerManager::Statistics GetLogStatistics() {
    return LoggerManager::Instance().GetStatistics();
}

} // namespace logger
} // namespace rediscc
