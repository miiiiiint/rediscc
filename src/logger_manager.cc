#include "logger_manager.h"
#include "logger_utils.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <spdlog/async.h>
#include <spdlog/fmt/fmt.h>

namespace rediscc {
  namespace logger {

    // ThreadLocalContext 实现
    thread_local LogContext ThreadLocalContext::context_;

    LogContext& ThreadLocalContext::GetContext() { return context_; }

    void ThreadLocalContext::SetContext( const LogContext& context ) { context_ = context; }

    void ThreadLocalContext::ClearContext() { context_.Clear(); }

    // LogContext 实现
    std::string LogContext::ToJson() const {
      std::string json  = "{";
      bool        first = true;

      auto add_field = [ & ]( const std::string& key, const std::string& value ) {
        if ( !value.empty() ) {
          if ( !first ) json += ",";
          json  += "\"" + key + "\":\"" + value + "\"";
          first  = false;
        }
      };

      add_field( "request_id", request_id );
      add_field( "session_id", session_id );
      add_field( "user_id", user_id );
      add_field( "operation", operation );

      for ( const auto& field : custom_fields ) {
        add_field( field.first, field.second );
      }

      json += "}";
      return json;
    }

    void LogContext::Clear() {
      request_id.clear();
      session_id.clear();
      user_id.clear();
      operation.clear();
      custom_fields.clear();
    }

    // SensitiveFilter 实现
    SensitiveFilter::SensitiveFilter( const FilterConfig& config )
        : config_( config ) {
      CompilePatterns();
    }

    void SensitiveFilter::CompilePatterns() {
      patterns_.clear();
      for ( const auto& pattern : config_.sensitive_patterns ) {
        try {
          patterns_.emplace_back( pattern, std::regex_constants::icase );
        } catch ( const std::exception& e ) {
          std::cerr << "Invalid regex pattern: " << pattern << " - " << e.what() << std::endl;
        }
      }
    }

    std::string SensitiveFilter::Filter( const std::string& message ) const {
      if ( !config_.enabled ) {
        return message;
      }

      std::lock_guard< std::mutex > lock( mutex_ );
      return MaskSensitiveData( message );
    }

    std::string SensitiveFilter::MaskSensitiveData( const std::string& input ) const {
      std::string result = input;

      // 过滤敏感关键词
      for ( const auto& key : config_.sensitive_keys ) {
        size_t      pos            = 0;
        std::string search_pattern = key + "=";
        while ( ( pos = result.find( search_pattern, pos ) ) != std::string::npos ) {
          size_t value_start = pos + search_pattern.length();
          size_t value_end   = result.find_first_of( " \t\n\r,;", value_start );
          if ( value_end == std::string::npos ) {
            value_end = result.length();
          }
          result.replace( value_start, value_end - value_start, config_.mask_string );
          pos = value_start + config_.mask_string.length();
        }
      }

      // 使用正则表达式过滤
      for ( const auto& pattern : patterns_ ) {
        result = std::regex_replace( result, pattern, config_.mask_string );
      }

      return result;
    }

    void SensitiveFilter::UpdateConfig( const FilterConfig& config ) {
      std::lock_guard< std::mutex > lock( mutex_ );
      config_ = config;
      CompilePatterns();
    }

    // JsonFormatter 实现
    JsonFormatter::JsonFormatter( const ContextConfig& context_config )
        : context_config_( context_config ) {}

    void JsonFormatter::format( const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest ) {
      std::string json_str = FormatJson( msg );
      dest.append( json_str.data(), json_str.data() + json_str.size() );
    }

    std::unique_ptr< spdlog::formatter > JsonFormatter::clone() const {
      return std::make_unique< JsonFormatter >( context_config_ );
    }

    std::string JsonFormatter::FormatJson( const spdlog::details::log_msg& msg ) const {
      std::string json = "{";

      // 时间戳
      if ( context_config_.include_timestamp ) {
        auto time_t = std::chrono::system_clock::to_time_t( msg.time );
        auto ms
            = std::chrono::duration_cast< std::chrono::milliseconds >( msg.time.time_since_epoch() )
              % 1000;

        std::tm tm_buf;
#ifdef _WIN32
        localtime_s( &tm_buf, &time_t );
#else
        localtime_r( &time_t, &tm_buf );
#endif

        char time_str[ 64 ];
        std::strftime( time_str, sizeof( time_str ), "%Y-%m-%d %H:%M:%S", &tm_buf );

        json += fmt::format( "\"timestamp\":\"{}.{:03d}\",", time_str, ms.count() );
      }

      // 日志级别
      json += fmt::format( "\"level\":\"{}\",", spdlog::level::to_string_view( msg.level ) );

      // 线程ID
      if ( context_config_.include_thread_id ) {
        json += fmt::format( "\"thread_id\":\"{}\",", msg.thread_id );
      }

      // 进程ID
      if ( context_config_.include_process_id ) {
        json += fmt::format( "\"process_id\":\"{}\",",
                             std::hash< std::thread::id > {}( std::this_thread::get_id() ) );
      }

      // 日志消息
      std::string message( msg.payload.data(), msg.payload.size() );
      // 转义JSON特殊字符
      size_t      pos = 0;
      while ( ( pos = message.find( "\"", pos ) ) != std::string::npos ) {
        message.replace( pos, 1, "\\\"" );
        pos += 2;
      }
      while ( ( pos = message.find( "\n", pos ) ) != std::string::npos ) {
        message.replace( pos, 1, "\\n" );
        pos += 2;
      }

      json += fmt::format( "\"message\":\"{}\"", message );

      // 上下文信息
      const auto& context = ThreadLocalContext::GetContext();
      if ( !context.request_id.empty() || !context.session_id.empty() || !context.user_id.empty()
           || !context.operation.empty() || !context.custom_fields.empty() ) {
        json += ",\"context\":" + context.ToJson();
      }

      json += "}";
      return json;
    }

    // LoggerManager 实现
    LoggerManager& LoggerManager::Instance() {
      static LoggerManager instance;
      return instance;
    }

    bool LoggerManager::Initialize( const LoggerConfig& config ) {
      std::lock_guard< std::mutex > lock( mutex_ );

      if ( initialized_ ) {
        Shutdown();
      }

      config_ = config;

      if ( !config_.Validate() ) {
        std::cerr << "Invalid logger configuration" << std::endl;
        return false;
      }

      try {
        // 创建日志目录
        for ( const auto& sink : config_.sinks ) {
          if ( !sink.filename.empty() ) {
            std::filesystem::path log_path( sink.filename );
            std::filesystem::create_directories( log_path.parent_path() );
          }
        }

        // 设置异步日志
        if ( config_.async_config.enabled ) {
          if ( !SetupAsyncLogging( config_.async_config ) ) {
            return false;
          }
        }

        // 创建sinks
        if ( !CreateSinks( config_ ) ) {
          return false;
        }

        // 创建过滤器
        filter_ = std::make_unique< SensitiveFilter >( config_.filter_config );

        // 重置统计信息
        ResetStatistics();

        initialized_ = true;
        return true;

      } catch ( const std::exception& e ) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return false;
      }
    }

    bool LoggerManager::Initialize( const std::string& config_file ) {
      LoggerConfig config;

      if ( !config_file.empty() ) {
        if ( !config.LoadFromFile( config_file ) ) {
          std::cerr << "Failed to load config from file: " << config_file << std::endl;
          // 继续使用默认配置
        }
      }

      // 从环境变量加载配置
      config.LoadFromEnv();

      return Initialize( config );
    }

    void LoggerManager::Shutdown() {
      std::lock_guard< std::mutex > lock( mutex_ );

      if ( logger_ ) {
        logger_->flush();
        spdlog::drop( config_.logger_name );
        logger_.reset();
      }

      if ( config_.async_config.enabled ) {
        spdlog::shutdown();
      }

      filter_.reset();
      initialized_ = false;
    }

    bool LoggerManager::SetupAsyncLogging( const AsyncConfig& async_config ) {
      try {
        spdlog::init_thread_pool( async_config.queue_size, async_config.thread_count );
        return true;
      } catch ( const std::exception& e ) {
        std::cerr << "Failed to setup async logging: " << e.what() << std::endl;
        return false;
      }
    }

    bool LoggerManager::CreateSinks( const LoggerConfig& config ) {
      std::vector< spdlog::sink_ptr > sinks;

      for ( const auto& sink_config : config.sinks ) {
        spdlog::sink_ptr sink;

        try {
          if ( sink_config.type == "console" ) {
            if ( sink_config.colored ) {
              sink = std::make_shared< spdlog::sinks::stdout_color_sink_mt >();
            } else {
              auto stdout_sink = std::make_shared< spdlog::sinks::stdout_color_sink_mt >();
              stdout_sink->set_color_mode( spdlog::color_mode::never );
              sink = stdout_sink;
            }
          } else if ( sink_config.type == "file" ) {
            sink = std::make_shared< spdlog::sinks::basic_file_sink_mt >( sink_config.filename );
          } else if ( sink_config.type == "rotating_file" ) {
            sink = std::make_shared< spdlog::sinks::rotating_file_sink_mt >(
                sink_config.filename, sink_config.max_size, sink_config.max_files );
          } else if ( sink_config.type == "daily_file" ) {
            sink = std::make_shared< spdlog::sinks::daily_file_sink_mt >(
                sink_config.filename, sink_config.rotation_hour, sink_config.rotation_minute );
          } else {
            std::cerr << "Unknown sink type: " << sink_config.type << std::endl;
            continue;
          }

          // 设置格式
          if ( config.format == LogFormat::JSON ) {
            sink->set_formatter( std::make_unique< JsonFormatter >( config.context_config ) );
          } else {
            sink->set_pattern( sink_config.pattern );
          }

          // 设置级别
          sink->set_level( config.ToSpdlogLevel( sink_config.level ) );

          sinks.push_back( sink );

        } catch ( const std::exception& e ) {
          std::cerr << "Failed to create sink: " << sink_config.type << " - " << e.what()
                    << std::endl;
          return false;
        }
      }

      if ( sinks.empty() ) {
        std::cerr << "No valid sinks created" << std::endl;
        return false;
      }

      // 创建logger
      try {
        if ( config.async_config.enabled ) {
          logger_
              = std::make_shared< spdlog::async_logger >( config.logger_name,
                                                          sinks.begin(),
                                                          sinks.end(),
                                                          spdlog::thread_pool(),
                                                          spdlog::async_overflow_policy::block );
        } else {
          logger_ = std::make_shared< spdlog::logger >(
              config.logger_name, sinks.begin(), sinks.end() );
        }

        logger_->set_level( config.ToSpdlogLevel( config.global_level ) );

        if ( config.auto_flush ) {
          logger_->flush_on( spdlog::level::err );
          spdlog::flush_every( std::chrono::milliseconds( config.flush_interval_ms ) );
        }

        spdlog::register_logger( logger_ );
        return true;

      } catch ( const std::exception& e ) {
        std::cerr << "Failed to create logger: " << e.what() << std::endl;
        return false;
      }
    }

    std::string LoggerManager::ProcessMessage( const std::string& message ) const {
      if ( filter_ ) {
        return filter_->Filter( message );
      }
      return message;
    }

    void LoggerManager::SetLogLevel( LogLevel level ) {
      std::lock_guard< std::mutex > lock( mutex_ );
      if ( logger_ ) {
        logger_->set_level( config_.ToSpdlogLevel( level ) );
        config_.global_level = level;
      }
    }

    LogLevel LoggerManager::GetLogLevel() const {
      std::lock_guard< std::mutex > lock( mutex_ );
      return config_.global_level;
    }

    void LoggerManager::UpdateConfig( const LoggerConfig& config ) { Initialize( config ); }

    const LoggerConfig& LoggerManager::GetConfig() const {
      std::lock_guard< std::mutex > lock( mutex_ );
      return config_;
    }

    void LoggerManager::SetContext( const LogContext& context ) {
      ThreadLocalContext::SetContext( context );
    }

    LogContext LoggerManager::GetContext() const { return ThreadLocalContext::GetContext(); }

    void LoggerManager::ClearContext() { ThreadLocalContext::ClearContext(); }

    LoggerManager::Statistics LoggerManager::GetStatistics() const {
      std::lock_guard< std::mutex > lock( stats_mutex_ );
      return stats_;
    }

    void LoggerManager::ResetStatistics() {
      std::lock_guard< std::mutex > lock( stats_mutex_ );
      stats_ = Statistics {};
    }

    void LoggerManager::RecordLogStatistics( LogLevel level, size_t message_size ) const {
      LogStatisticsCollector::Instance().RecordLog( level, message_size );
    }

  }  // namespace logger
}  // namespace rediscc
