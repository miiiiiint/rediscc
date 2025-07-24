#include "logger_utils.h"
#include "logger_manager.h"
#include <iomanip>
#include <sstream>

// Define local logging macros to avoid circular dependency
#define LOCAL_DEBUG( ... ) rediscc::logger::LoggerManager::Instance().Debug( __VA_ARGS__ )
#define LOCAL_INFO( ... )  rediscc::logger::LoggerManager::Instance().Info( __VA_ARGS__ )
#define LOCAL_ERROR( ... ) rediscc::logger::LoggerManager::Instance().Error( __VA_ARGS__ )

namespace rediscc {
  namespace logger {

    // ScopedLogContext 实现
    ScopedLogContext::ScopedLogContext( const LogContext& context )
        : previous_context_( ThreadLocalContext::GetContext() ) {
      ThreadLocalContext::SetContext( context );
    }

    ScopedLogContext::~ScopedLogContext() { ThreadLocalContext::SetContext( previous_context_ ); }

    // PerformanceTimer 实现
    PerformanceTimer::PerformanceTimer( const std::string& operation_name )
        : operation_name_( operation_name )
        , start_time_( std::chrono::high_resolution_clock::now() )
        , stopped_( false ) {
      LOCAL_DEBUG( "PERF_START: {}", operation_name_ );
    }

    PerformanceTimer::~PerformanceTimer() {
      if ( !stopped_ ) {
        Stop();
      }
    }

    void PerformanceTimer::Stop() {
      if ( !stopped_ ) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration
            = std::chrono::duration_cast< std::chrono::microseconds >( end_time - start_time_ );

        LOCAL_INFO( "PERF_END: {} took {:.3f} ms", operation_name_, duration.count() / 1000.0 );
        stopped_ = true;
      }
    }

    double PerformanceTimer::GetElapsedMs() const {
      auto current_time = std::chrono::high_resolution_clock::now();
      auto duration
          = std::chrono::duration_cast< std::chrono::microseconds >( current_time - start_time_ );
      return duration.count() / 1000.0;
    }

    // RequestTracker 实现
    RequestTracker::RequestTracker( const std::string& request_id )
        : request_id_( request_id )
        , start_time_( std::chrono::high_resolution_clock::now() ) {
      context_.request_id = request_id;
      scoped_context_     = std::make_unique< ScopedLogContext >( context_ );

      LOCAL_INFO( "REQUEST_START: {}", request_id_ );
    }

    RequestTracker::RequestTracker( const std::string& request_id, const std::string& user_id )
        : RequestTracker( request_id ) {
      context_.user_id = user_id;
      scoped_context_  = std::make_unique< ScopedLogContext >( context_ );
    }

    RequestTracker::~RequestTracker() {
      auto end_time = std::chrono::high_resolution_clock::now();
      auto duration
          = std::chrono::duration_cast< std::chrono::milliseconds >( end_time - start_time_ );

      LOCAL_INFO( "REQUEST_END: {} completed in {} ms", request_id_, duration.count() );
    }

    void RequestTracker::SetOperation( const std::string& operation ) {
      context_.operation = operation;
      scoped_context_    = std::make_unique< ScopedLogContext >( context_ );

      LOCAL_DEBUG( "REQUEST_OPERATION: {} - {}", request_id_, operation );
    }

    void RequestTracker::AddField( const std::string& key, const std::string& value ) {
      context_.custom_fields[ key ] = value;
      scoped_context_               = std::make_unique< ScopedLogContext >( context_ );
    }

    void RequestTracker::LogMilestone( const std::string& milestone ) {
      auto current_time = std::chrono::high_resolution_clock::now();
      auto elapsed
          = std::chrono::duration_cast< std::chrono::milliseconds >( current_time - start_time_ );

      LOCAL_INFO( "REQUEST_MILESTONE: {} - {} at +{}ms", request_id_, milestone, elapsed.count() );
    }

    void RequestTracker::LogError( const std::string& error_msg ) {
      LOCAL_ERROR( "REQUEST_ERROR: {} - {}", request_id_, error_msg );
    }

    // LogLevelGuard 实现
    LogLevelGuard::LogLevelGuard( LogLevel temp_level )
        : original_level_( LoggerManager::Instance().GetLogLevel() ) {
      LoggerManager::Instance().SetLogLevel( temp_level );
    }

    LogLevelGuard::~LogLevelGuard() { LoggerManager::Instance().SetLogLevel( original_level_ ); }

    // BatchLogger 实现
    BatchLogger::BatchLogger( size_t batch_size )
        : batch_size_( batch_size )
        , auto_flush_interval_( std::chrono::milliseconds::zero() )
        , last_flush_( std::chrono::system_clock::now() ) {
      batch_.reserve( batch_size_ );
    }

    BatchLogger::~BatchLogger() { Flush(); }

    void BatchLogger::Flush() {
      std::lock_guard< std::mutex > lock( mutex_ );
      FlushInternal();
    }

    void BatchLogger::SetAutoFlushInterval( std::chrono::milliseconds interval ) {
      std::lock_guard< std::mutex > lock( mutex_ );
      auto_flush_interval_ = interval;
    }

    void BatchLogger::FlushInternal() {
      if ( batch_.empty() ) {
        return;
      }

      for ( const auto& entry : batch_ ) {
        LoggerManager::Instance().Log( entry.level, entry.message );
      }

      batch_.clear();
      last_flush_ = std::chrono::system_clock::now();
    }

    // LogStatisticsCollector 实现
    LogStatisticsCollector& LogStatisticsCollector::Instance() {
      static LogStatisticsCollector instance;
      return instance;
    }

    void LogStatisticsCollector::RecordLog( LogLevel level, size_t message_size ) {
      std::lock_guard< std::mutex > lock( mutex_ );

      if ( stats_.total_logs == 0 ) {
        stats_.start_time = std::chrono::system_clock::now();
      }

      stats_.level_counts[ level ]++;
      stats_.total_logs++;
      stats_.total_bytes      += message_size;
      stats_.avg_message_size  = static_cast< double >( stats_.total_bytes ) / stats_.total_logs;
      stats_.last_log_time     = std::chrono::system_clock::now();
    }

    void LogStatisticsCollector::RecordError( const std::string& error_type ) {
      std::lock_guard< std::mutex > lock( mutex_ );
      stats_.error_counts[ error_type ]++;
    }

    LogStatisticsCollector::DetailedStatistics LogStatisticsCollector::GetStatistics() const {
      std::lock_guard< std::mutex > lock( mutex_ );
      return stats_;
    }

    void LogStatisticsCollector::ResetStatistics() {
      std::lock_guard< std::mutex > lock( mutex_ );
      stats_ = DetailedStatistics {};
    }

    std::string LogStatisticsCollector::GenerateReport() const {
      std::lock_guard< std::mutex > lock( mutex_ );
      std::ostringstream            oss;

      oss << "=== Log Statistics Report ===\n";
      oss << "Total Logs: " << stats_.total_logs << "\n";
      oss << "Total Bytes: " << stats_.total_bytes << "\n";
      oss << "Average Message Size: " << std::fixed << std::setprecision( 2 )
          << stats_.avg_message_size << " bytes\n";

      if ( stats_.total_logs > 0 ) {
        auto duration = std::chrono::duration_cast< std::chrono::seconds >( stats_.last_log_time
                                                                            - stats_.start_time );
        oss << "Duration: " << duration.count() << " seconds\n";
        oss << "Logs per second: " << std::fixed << std::setprecision( 2 )
            << static_cast< double >( stats_.total_logs ) / duration.count() << "\n";
      }

      oss << "\nLogs by Level:\n";
      for ( const auto& [ level, count ] : stats_.level_counts ) {
        oss << "  " << static_cast< int >( level ) << ": " << count << "\n";
      }

      if ( !stats_.error_counts.empty() ) {
        oss << "\nErrors by Type:\n";
        for ( const auto& [ error_type, count ] : stats_.error_counts ) {
          oss << "  " << error_type << ": " << count << "\n";
        }
      }

      return oss.str();
    }

  }  // namespace logger
}  // namespace rediscc
