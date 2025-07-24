#ifndef REDISCC_LOGGER_MANAGER_H
#define REDISCC_LOGGER_MANAGER_H

#include "logger_config.h"
#include <memory>
#include <mutex>
#include <regex>
#include <spdlog/async.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_map>

namespace rediscc {
  namespace logger {

    // 前向声明
    class LogStatisticsCollector;

    /**
     * @brief 日志上下文信息
     */
    struct LogContext {
      std::string                                    request_id;
      std::string                                    session_id;
      std::string                                    user_id;
      std::string                                    operation;
      std::unordered_map< std::string, std::string > custom_fields;

      LogContext() = default;
      LogContext( const std::string& req_id )
          : request_id( req_id ) {}

      std::string ToJson() const;
      void        Clear();
    };

    /**
     * @brief 线程本地日志上下文
     */
    class ThreadLocalContext {
    public:
      static LogContext& GetContext();
      static void        SetContext( const LogContext& context );
      static void        ClearContext();

    private:
      static thread_local LogContext context_;
    };

    /**
     * @brief 敏感信息过滤器
     */
    class SensitiveFilter {
    public:
      explicit SensitiveFilter( const FilterConfig& config );

      std::string Filter( const std::string& message ) const;
      void        UpdateConfig( const FilterConfig& config );

    private:
      FilterConfig              config_;
      std::vector< std::regex > patterns_;
      mutable std::mutex        mutex_;

      void        CompilePatterns();
      std::string MaskSensitiveData( const std::string& input ) const;
    };

    /**
     * @brief 自定义JSON格式化器
     */
    class JsonFormatter : public spdlog::formatter {
    public:
      explicit JsonFormatter( const ContextConfig& context_config );

      void format( const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest ) override;
      std::unique_ptr< spdlog::formatter > clone() const override;

    private:
      ContextConfig context_config_;
      std::string   FormatJson( const spdlog::details::log_msg& msg ) const;
    };

    /**
     * @brief 日志管理器主类
     */
    class LoggerManager {
    public:
      static LoggerManager& Instance();

      // 初始化和配置
      bool Initialize( const LoggerConfig& config );
      bool Initialize( const std::string& config_file = "" );
      void Shutdown();

      // 日志记录接口
      template < typename... Args >
      void Log( LogLevel level, const std::string& format, Args&&... args );

      template < typename... Args >
      void LogWithContext( LogLevel           level,
                           const LogContext&  context,
                           const std::string& format,
                           Args&&... args );

      // 便捷接口
      template < typename... Args > void Trace( const std::string& format, Args&&... args );

      template < typename... Args > void Debug( const std::string& format, Args&&... args );

      template < typename... Args > void Info( const std::string& format, Args&&... args );

      template < typename... Args > void Warn( const std::string& format, Args&&... args );

      template < typename... Args > void Error( const std::string& format, Args&&... args );

      template < typename... Args > void Critical( const std::string& format, Args&&... args );

      // 配置管理
      void                SetLogLevel( LogLevel level );
      LogLevel            GetLogLevel() const;
      void                UpdateConfig( const LoggerConfig& config );
      const LoggerConfig& GetConfig() const;

      // 上下文管理
      void       SetContext( const LogContext& context );
      LogContext GetContext() const;
      void       ClearContext();

      // 性能统计
      struct Statistics {
        size_t total_logs;
        size_t filtered_logs;
        size_t error_count;
        double avg_log_time_ms;
      };
      Statistics GetStatistics() const;
      void       ResetStatistics();

    private:
      LoggerManager()                                  = default;
      ~LoggerManager()                                 = default;
      LoggerManager( const LoggerManager& )            = delete;
      LoggerManager& operator=( const LoggerManager& ) = delete;

      bool        CreateSinks( const LoggerConfig& config );
      bool        SetupAsyncLogging( const AsyncConfig& async_config );
      std::string ProcessMessage( const std::string& message ) const;
      void        RecordLogStatistics( LogLevel level, size_t message_size ) const;

      std::shared_ptr< spdlog::logger >  logger_;
      LoggerConfig                       config_;
      std::unique_ptr< SensitiveFilter > filter_;
      mutable std::mutex                 mutex_;
      bool                               initialized_;

      // 统计信息
      mutable Statistics stats_;
      mutable std::mutex stats_mutex_;
    };

    // 模板方法实现
    template < typename... Args >
    void LoggerManager::Log( LogLevel level, const std::string& format, Args&&... args ) {
      if ( !initialized_ || !logger_ ) return;

      auto start_time = std::chrono::high_resolution_clock::now();

      try {
        // Use a workaround for spdlog's compile-time format string requirement
        // Format the message manually using a simple approach
        std::ostringstream oss;
        oss << format;

        // Simple parameter substitution for basic cases
        if constexpr ( sizeof...( args ) > 0 ) {
          oss << " [";
          ( ( oss << args << " " ), ... );  // Fold expression
          oss << "]";
        }

        std::string processed_msg = ProcessMessage( oss.str() );
        logger_->log( LoggerConfig::ToSpdlogLevelStatic( level ), processed_msg );

        // 更新统计
        std::lock_guard< std::mutex > lock( stats_mutex_ );
        stats_.total_logs++;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration
            = std::chrono::duration_cast< std::chrono::microseconds >( end_time - start_time );
        stats_.avg_log_time_ms
            = ( stats_.avg_log_time_ms * ( stats_.total_logs - 1 ) + duration.count() / 1000.0 )
              / stats_.total_logs;

        // 更新全局统计收集器（通过辅助函数避免循环依赖）
        RecordLogStatistics( level, processed_msg.size() );
      } catch ( const std::exception& e ) {
        std::lock_guard< std::mutex > lock( stats_mutex_ );
        stats_.error_count++;
      }
    }

    template < typename... Args >
    void LoggerManager::LogWithContext( LogLevel           level,
                                        const LogContext&  context,
                                        const std::string& format,
                                        Args&&... args ) {
      ThreadLocalContext::SetContext( context );
      Log( level, format, std::forward< Args >( args )... );
    }

    template < typename... Args >
    void LoggerManager::Trace( const std::string& format, Args&&... args ) {
      Log( LogLevel::TRACE, format, std::forward< Args >( args )... );
    }

    template < typename... Args >
    void LoggerManager::Debug( const std::string& format, Args&&... args ) {
      Log( LogLevel::DEBUG, format, std::forward< Args >( args )... );
    }

    template < typename... Args >
    void LoggerManager::Info( const std::string& format, Args&&... args ) {
      Log( LogLevel::INFO, format, std::forward< Args >( args )... );
    }

    template < typename... Args >
    void LoggerManager::Warn( const std::string& format, Args&&... args ) {
      Log( LogLevel::WARN, format, std::forward< Args >( args )... );
    }

    template < typename... Args >
    void LoggerManager::Error( const std::string& format, Args&&... args ) {
      Log( LogLevel::ERROR, format, std::forward< Args >( args )... );
    }

    template < typename... Args >
    void LoggerManager::Critical( const std::string& format, Args&&... args ) {
      Log( LogLevel::CRITICAL, format, std::forward< Args >( args )... );
    }

  }  // namespace logger
}  // namespace rediscc

#endif  // REDISCC_LOGGER_MANAGER_H
