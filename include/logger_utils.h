#ifndef REDISCC_LOGGER_UTILS_H
#define REDISCC_LOGGER_UTILS_H

#include "logger_manager.h"
#include <chrono>
#include <memory>
#include <sstream>
#include <string>

namespace rediscc {
  namespace logger {

    /**
     * @brief RAII风格的日志上下文管理器
     */
    class ScopedLogContext {
    public:
      explicit ScopedLogContext( const LogContext& context );
      ~ScopedLogContext();

      ScopedLogContext( const ScopedLogContext& )            = delete;
      ScopedLogContext& operator=( const ScopedLogContext& ) = delete;

    private:
      LogContext previous_context_;
    };

    /**
     * @brief 性能计时器，自动记录执行时间
     */
    class PerformanceTimer {
    public:
      explicit PerformanceTimer( const std::string& operation_name );
      ~PerformanceTimer();

      // 手动停止计时并记录
      void Stop();

      // 获取当前经过的时间（毫秒）
      double GetElapsedMs() const;

    private:
      std::string                                    operation_name_;
      std::chrono::high_resolution_clock::time_point start_time_;
      bool                                           stopped_;
    };

    /**
     * @brief 请求追踪器，为整个请求生命周期提供统一的上下文
     */
    class RequestTracker {
    public:
      explicit RequestTracker( const std::string& request_id );
      RequestTracker( const std::string& request_id, const std::string& user_id );
      ~RequestTracker();

      // 设置操作名称
      void SetOperation( const std::string& operation );

      // 添加自定义字段
      void AddField( const std::string& key, const std::string& value );

      // 记录里程碑事件
      void LogMilestone( const std::string& milestone );

      // 记录错误
      void LogError( const std::string& error_msg );

      // 获取请求ID
      const std::string& GetRequestId() const { return request_id_; }

    private:
      std::string                                    request_id_;
      LogContext                                     context_;
      std::chrono::high_resolution_clock::time_point start_time_;
      std::unique_ptr< ScopedLogContext >            scoped_context_;
    };

    /**
     * @brief 日志级别守卫，临时改变日志级别
     */
    class LogLevelGuard {
    public:
      explicit LogLevelGuard( LogLevel temp_level );
      ~LogLevelGuard();

    private:
      LogLevel original_level_;
    };

    /**
     * @brief 批量日志记录器，用于高频日志场景
     */
    class BatchLogger {
    public:
      explicit BatchLogger( size_t batch_size = 100 );
      ~BatchLogger();

      // 添加日志到批次
      template < typename... Args >
      void Add( LogLevel level, const std::string& format, Args&&... args );

      // 强制刷新批次
      void Flush();

      // 设置自动刷新间隔
      void SetAutoFlushInterval( std::chrono::milliseconds interval );

    private:
      struct LogEntry {
        LogLevel                              level;
        std::string                           message;
        std::chrono::system_clock::time_point timestamp;
      };

      std::vector< LogEntry >               batch_;
      size_t                                batch_size_;
      std::chrono::milliseconds             auto_flush_interval_;
      std::chrono::system_clock::time_point last_flush_;
      mutable std::mutex                    mutex_;

      void FlushInternal();
    };

    /**
     * @brief 日志统计收集器
     */
    class LogStatisticsCollector {
    public:
      static LogStatisticsCollector& Instance();

      // 记录日志事件
      void RecordLog( LogLevel level, size_t message_size );

      // 记录错误
      void RecordError( const std::string& error_type );

      // 获取统计信息
      struct DetailedStatistics {
        std::map< LogLevel, size_t >          level_counts;
        std::map< std::string, size_t >       error_counts;
        size_t                                total_logs;
        size_t                                total_bytes;
        double                                avg_message_size;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point last_log_time;
      };

      DetailedStatistics GetStatistics() const;
      void               ResetStatistics();

      // 生成统计报告
      std::string GenerateReport() const;

    private:
      LogStatisticsCollector() = default;

      mutable std::mutex mutex_;
      DetailedStatistics stats_;
    };

// 便捷宏定义
#define REDISCC_SCOPED_CONTEXT( context ) rediscc::logger::ScopedLogContext _scoped_ctx( context )

#define REDISCC_PERF_TIMER( operation ) rediscc::logger::PerformanceTimer _perf_timer( operation )

#define REDISCC_REQUEST_TRACKER( request_id )                                                      \
  rediscc::logger::RequestTracker _req_tracker( request_id )

#define REDISCC_LOG_LEVEL_GUARD( level ) rediscc::logger::LogLevelGuard _level_guard( level )

    // 模板方法实现
    template < typename... Args >
    void BatchLogger::Add( LogLevel level, const std::string& format, Args&&... args ) {
      std::lock_guard< std::mutex > lock( mutex_ );

      LogEntry entry;
      entry.level = level;

      // Use a simple approach to avoid fmt::format consteval issues
      std::ostringstream oss;
      oss << format;
      if constexpr ( sizeof...( args ) > 0 ) {
        oss << " [";
        ( ( oss << args << " " ), ... );  // Fold expression
        oss << "]";
      }
      entry.message = oss.str();

      entry.timestamp = std::chrono::system_clock::now();

      batch_.push_back( std::move( entry ) );

      // 检查是否需要刷新
      if ( batch_.size() >= batch_size_
           || ( auto_flush_interval_.count() > 0
                && std::chrono::duration_cast< std::chrono::milliseconds >( entry.timestamp
                                                                            - last_flush_ )
                       >= auto_flush_interval_ ) ) {
        FlushInternal();
      }
    }

  }  // namespace logger
}  // namespace rediscc

#endif  // REDISCC_LOGGER_UTILS_H
