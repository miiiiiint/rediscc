#ifndef REDISCC_LOG_H
#define REDISCC_LOG_H

#include "logger_manager.h"
#include <string>

namespace rediscc {
  namespace logger {

    /**
     * @brief 初始化日志系统
     * @param config_file 配置文件路径，为空则使用默认配置
     * @return 初始化是否成功
     */
    bool InitLogger( const std::string& config_file = "" );

    /**
     * @brief 关闭日志系统
     */
    void ShutdownLogger();

    /**
     * @brief 设置日志上下文
     * @param context 日志上下文信息
     */
    void SetLogContext( const LogContext& context );

    /**
     * @brief 清除日志上下文
     */
    void ClearLogContext();

    /**
     * @brief 获取日志统计信息
     */
    LoggerManager::Statistics GetLogStatistics();

  }  // namespace logger
}  // namespace rediscc

// 兼容性宏定义 - 保持向后兼容
#define REDISCC_TRACE( ... )    rediscc::logger::LoggerManager::Instance().Trace( __VA_ARGS__ )
#define REDISCC_DEBUG( ... )    rediscc::logger::LoggerManager::Instance().Debug( __VA_ARGS__ )
#define REDISCC_INFO( ... )     rediscc::logger::LoggerManager::Instance().Info( __VA_ARGS__ )
#define REDISCC_WARN( ... )     rediscc::logger::LoggerManager::Instance().Warn( __VA_ARGS__ )
#define REDISCC_ERROR( ... )    rediscc::logger::LoggerManager::Instance().Error( __VA_ARGS__ )
#define REDISCC_CRITICAL( ... ) rediscc::logger::LoggerManager::Instance().Critical( __VA_ARGS__ )

// 带上下文的日志宏
#define REDISCC_LOG_WITH_CONTEXT( level, context, ... )                                            \
  rediscc::logger::LoggerManager::Instance().LogWithContext( level, context, __VA_ARGS__ )

// 带文件和行号的调试宏
#ifdef _DEBUG
#  define REDISCC_DEBUG_LOCATION( ... )                                                            \
    REDISCC_DEBUG( "[{}:{}] {}", __FILE__, __LINE__, fmt::format( __VA_ARGS__ ) )
#else
#  define REDISCC_DEBUG_LOCATION( ... )
#endif

// 性能日志宏（带时间测量）
#define REDISCC_PERF_LOG( operation, ... )                                                         \
  do {                                                                                             \
    auto start = std::chrono::high_resolution_clock::now();                                        \
    __VA_ARGS__;                                                                                   \
    auto end      = std::chrono::high_resolution_clock::now();                                     \
    auto duration = std::chrono::duration_cast< std::chrono::microseconds >( end - start );        \
    REDISCC_INFO( "PERF: {} took {} microseconds", operation, duration.count() );                  \
  } while ( 0 )

// 条件日志宏
#define REDISCC_LOG_IF( condition, level, ... )                                                    \
  do {                                                                                             \
    if ( condition ) {                                                                             \
      rediscc::logger::LoggerManager::Instance().Log( level, __VA_ARGS__ );                        \
    }                                                                                              \
  } while ( 0 )

// 错误码日志宏
#define REDISCC_ERROR_CODE( error_code, ... )                                                      \
  REDISCC_ERROR( "ERROR_CODE: {} - {}", error_code, fmt::format( __VA_ARGS__ ) )

// 审计日志宏
#define REDISCC_AUDIT( user_id, operation, message )                                               \
  REDISCC_INFO( "AUDIT: user={} operation={} message={}", user_id, operation, message )

#endif  // REDISCC_LOG_H
