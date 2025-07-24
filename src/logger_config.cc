#include "logger_config.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace rediscc {
  namespace logger {

    LoggerConfig::LoggerConfig() { SetDefaults(); }

    void LoggerConfig::SetDefaults() {
      logger_name       = "REDISCC";
      global_level      = LogLevel::INFO;
      format            = LogFormat::TEXT;
      auto_flush        = true;
      flush_interval_ms = 1000;

      // 默认添加控制台和文件输出
      SinkConfig console_sink;
      console_sink.type    = "console";
      console_sink.pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v";
      console_sink.level   = LogLevel::INFO;
      console_sink.colored = true;

      SinkConfig file_sink;
      file_sink.type      = "rotating_file";
      file_sink.pattern   = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v";
      file_sink.level     = LogLevel::DEBUG;
      file_sink.filename  = "logs/rediscc.log";
      file_sink.max_size  = 10 * 1024 * 1024;  // 10MB
      file_sink.max_files = 5;

      sinks.clear();
      sinks.push_back( console_sink );
      sinks.push_back( file_sink );
    }

    LoggerConfig LoggerConfig::GetDefaultConfig() { return LoggerConfig(); }

    bool LoggerConfig::LoadFromEnv() {
      // 从环境变量加载配置
      const char* level_env = std::getenv( "REDISCC_LOG_LEVEL" );
      if ( level_env ) {
        global_level = StringToLogLevel( level_env );
      }

      const char* format_env = std::getenv( "REDISCC_LOG_FORMAT" );
      if ( format_env ) {
        std::string format_str( format_env );
        std::transform( format_str.begin(), format_str.end(), format_str.begin(), ::tolower );
        if ( format_str == "json" ) {
          format = LogFormat::JSON;
        } else if ( format_str == "text" ) {
          format = LogFormat::TEXT;
        }
      }

      const char* async_env = std::getenv( "REDISCC_LOG_ASYNC" );
      if ( async_env ) {
        std::string async_str( async_env );
        std::transform( async_str.begin(), async_str.end(), async_str.begin(), ::tolower );
        async_config.enabled = ( async_str == "true" || async_str == "1" );
      }

      const char* file_env = std::getenv( "REDISCC_LOG_FILE" );
      if ( file_env && !sinks.empty() ) {
        for ( auto& sink : sinks ) {
          if ( sink.type == "file" || sink.type == "rotating_file" ) {
            sink.filename = file_env;
            break;
          }
        }
      }

      return true;
    }

    bool LoggerConfig::LoadFromFile( const std::string& config_file ) {
      std::ifstream file( config_file );
      if ( !file.is_open() ) {
        std::cerr << "Failed to open config file: " << config_file << std::endl;
        return false;
      }

      std::string content( ( std::istreambuf_iterator< char >( file ) ),
                           std::istreambuf_iterator< char >() );
      file.close();

      return ParseJsonConfig( content );
    }

    bool LoggerConfig::ParseJsonConfig( const std::string& json_content ) {
      // 简化的JSON解析 - 在实际项目中应该使用专业的JSON库
      // 这里只是示例实现

      // 解析日志级别
      size_t level_pos = json_content.find( "\"level\"" );
      if ( level_pos != std::string::npos ) {
        size_t colon_pos   = json_content.find( ":", level_pos );
        size_t quote_start = json_content.find( "\"", colon_pos );
        size_t quote_end   = json_content.find( "\"", quote_start + 1 );
        if ( quote_start != std::string::npos && quote_end != std::string::npos ) {
          std::string level_str
              = json_content.substr( quote_start + 1, quote_end - quote_start - 1 );
          global_level = StringToLogLevel( level_str );
        }
      }

      // 解析异步配置
      size_t async_pos = json_content.find( "\"async\"" );
      if ( async_pos != std::string::npos ) {
        size_t enabled_pos = json_content.find( "\"enabled\"", async_pos );
        if ( enabled_pos != std::string::npos ) {
          size_t colon_pos   = json_content.find( ":", enabled_pos );
          size_t value_start = json_content.find_first_not_of( " \t", colon_pos + 1 );
          if ( value_start != std::string::npos ) {
            async_config.enabled = ( json_content.substr( value_start, 4 ) == "true" );
          }
        }
      }

      return true;
    }

    bool LoggerConfig::SaveToFile( const std::string& config_file ) const {
      std::ofstream file( config_file );
      if ( !file.is_open() ) {
        return false;
      }

      // 生成JSON配置
      file << "{\n";
      file << "  \"logger_name\": \"" << logger_name << "\",\n";
      file << "  \"level\": \"" << LogLevelToString( global_level ) << "\",\n";
      file << "  \"format\": \"" << ( format == LogFormat::JSON ? "json" : "text" ) << "\",\n";
      file << "  \"auto_flush\": " << ( auto_flush ? "true" : "false" ) << ",\n";
      file << "  \"flush_interval_ms\": " << flush_interval_ms << ",\n";

      file << "  \"async\": {\n";
      file << "    \"enabled\": " << ( async_config.enabled ? "true" : "false" ) << ",\n";
      file << "    \"queue_size\": " << async_config.queue_size << ",\n";
      file << "    \"thread_count\": " << async_config.thread_count << "\n";
      file << "  },\n";

      file << "  \"context\": {\n";
      file << "    \"include_thread_id\": "
           << ( context_config.include_thread_id ? "true" : "false" ) << ",\n";
      file << "    \"include_process_id\": "
           << ( context_config.include_process_id ? "true" : "false" ) << ",\n";
      file << "    \"include_function_name\": "
           << ( context_config.include_function_name ? "true" : "false" ) << ",\n";
      file << "    \"include_file_line\": "
           << ( context_config.include_file_line ? "true" : "false" ) << "\n";
      file << "  },\n";

      file << "  \"filter\": {\n";
      file << "    \"enabled\": " << ( filter_config.enabled ? "true" : "false" ) << ",\n";
      file << "    \"mask_string\": \"" << filter_config.mask_string << "\"\n";
      file << "  }\n";

      file << "}\n";
      file.close();

      return true;
    }

    bool LoggerConfig::Validate() const {
      if ( logger_name.empty() ) {
        return false;
      }

      if ( sinks.empty() ) {
        return false;
      }

      for ( const auto& sink : sinks ) {
        if ( sink.type.empty() ) {
          return false;
        }

        if ( ( sink.type == "file" || sink.type == "rotating_file" || sink.type == "daily_file" )
             && sink.filename.empty() ) {
          return false;
        }
      }

      return true;
    }

    LogLevel LoggerConfig::StringToLogLevel( const std::string& level_str ) const {
      std::string lower_str = level_str;
      std::transform( lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower );

      if ( lower_str == "trace" ) return LogLevel::TRACE;
      if ( lower_str == "debug" ) return LogLevel::DEBUG;
      if ( lower_str == "info" ) return LogLevel::INFO;
      if ( lower_str == "warn" || lower_str == "warning" ) return LogLevel::WARN;
      if ( lower_str == "error" ) return LogLevel::ERROR;
      if ( lower_str == "critical" ) return LogLevel::CRITICAL;
      if ( lower_str == "off" ) return LogLevel::OFF;

      return LogLevel::INFO;  // 默认级别
    }

    std::string LoggerConfig::LogLevelToString( LogLevel level ) const {
      switch ( level ) {
      case LogLevel::TRACE   : return "trace";
      case LogLevel::DEBUG   : return "debug";
      case LogLevel::INFO    : return "info";
      case LogLevel::WARN    : return "warn";
      case LogLevel::ERROR   : return "error";
      case LogLevel::CRITICAL: return "critical";
      case LogLevel::OFF     : return "off";
      default                : return "info";
      }
    }

    spdlog::level::level_enum LoggerConfig::ToSpdlogLevel( LogLevel level ) const {
      return ToSpdlogLevelStatic( level );
    }

    spdlog::level::level_enum LoggerConfig::ToSpdlogLevelStatic( LogLevel level ) {
      switch ( level ) {
      case LogLevel::TRACE   : return spdlog::level::trace;
      case LogLevel::DEBUG   : return spdlog::level::debug;
      case LogLevel::INFO    : return spdlog::level::info;
      case LogLevel::WARN    : return spdlog::level::warn;
      case LogLevel::ERROR   : return spdlog::level::err;
      case LogLevel::CRITICAL: return spdlog::level::critical;
      case LogLevel::OFF     : return spdlog::level::off;
      default                : return spdlog::level::info;
      }
    }

  }  // namespace logger
}  // namespace rediscc
