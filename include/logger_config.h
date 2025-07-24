#ifndef REDISCC_LOGGER_CONFIG_H
#define REDISCC_LOGGER_CONFIG_H

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace rediscc {
  namespace logger {

    /**
     * @brief 日志级别枚举
     */
    enum class LogLevel {
      TRACE    = 0,
      DEBUG    = 1,
      INFO     = 2,
      WARN     = 3,
      ERROR    = 4,
      CRITICAL = 5,
      OFF      = 6
    };

    /**
     * @brief 日志输出格式枚举
     */
    enum class LogFormat {
      TEXT,   // 普通文本格式
      JSON,   // JSON格式
      CUSTOM  // 自定义格式
    };

    /**
     * @brief 日志输出目标配置
     */
    struct SinkConfig {
      std::string type;     // "console", "file", "rotating_file", "daily_file"
      std::string pattern;  // 日志格式模式
      LogLevel    level;    // 输出级别
      bool        colored;  // 是否启用颜色（仅控制台）

      // 文件相关配置
      std::string filename;         // 文件名
      size_t      max_size;         // 最大文件大小（字节）
      size_t      max_files;        // 最大文件数量
      int         rotation_hour;    // 轮转小时（日志文件）
      int         rotation_minute;  // 轮转分钟

      SinkConfig()
          : type( "console" )
          , level( LogLevel::INFO )
          , colored( true )
          , max_size( 10 * 1024 * 1024 )
          , max_files( 3 )
          , rotation_hour( 0 )
          , rotation_minute( 0 ) {}
    };

    /**
     * @brief 异步日志配置
     */
    struct AsyncConfig {
      bool   enabled;                // 是否启用异步日志
      size_t queue_size;             // 队列大小
      size_t thread_count;           // 线程数量
      bool   overflow_policy_block;  // 队列满时是否阻塞

      AsyncConfig()
          : enabled( true )
          , queue_size( 8192 )
          , thread_count( 1 )
          , overflow_policy_block( false ) {}
    };

    /**
     * @brief 上下文信息配置
     */
    struct ContextConfig {
      bool include_thread_id;      // 包含线程ID
      bool include_process_id;     // 包含进程ID
      bool include_function_name;  // 包含函数名
      bool include_file_line;      // 包含文件名和行号
      bool include_timestamp;      // 包含时间戳

      ContextConfig()
          : include_thread_id( true )
          , include_process_id( false )
          , include_function_name( false )
          , include_file_line( false )
          , include_timestamp( true ) {}
    };

    /**
     * @brief 敏感信息过滤配置
     */
    struct FilterConfig {
      bool                       enabled;             // 是否启用过滤
      std::vector< std::string > sensitive_keys;      // 敏感字段关键词
      std::vector< std::string > sensitive_patterns;  // 敏感信息正则模式
      std::string                mask_string;         // 脱敏替换字符串

      FilterConfig()
          : enabled( true )
          , mask_string( "***" ) {
        sensitive_keys     = { "password", "passwd", "pwd", "secret", "key", "token", "auth" };
        sensitive_patterns = {
          R"(\b\d{11}\b)",                  // 手机号
          R"(\b[\w\.-]+@[\w\.-]+\.\w+\b)",  // 邮箱
          R"(\b\d{15,19}\b)"                // 银行卡号
        };
      }
    };

    /**
     * @brief 主日志配置类
     */
    class LoggerConfig {
    public:
      LoggerConfig();
      ~LoggerConfig() = default;

      // 基本配置
      std::string logger_name;
      LogLevel    global_level;
      LogFormat   format;
      bool        auto_flush;
      size_t      flush_interval_ms;

      // 子配置
      AsyncConfig               async_config;
      ContextConfig             context_config;
      FilterConfig              filter_config;
      std::vector< SinkConfig > sinks;

      // 配置加载和保存
      bool LoadFromFile( const std::string& config_file );
      bool LoadFromEnv();
      bool SaveToFile( const std::string& config_file ) const;

      // 配置验证
      bool Validate() const;

      // 获取默认配置
      static LoggerConfig GetDefaultConfig();

      // 辅助方法
      LogLevel                  StringToLogLevel( const std::string& level_str ) const;
      std::string               LogLevelToString( LogLevel level ) const;
      spdlog::level::level_enum ToSpdlogLevel( LogLevel level ) const;

      // 静态辅助方法
      static spdlog::level::level_enum ToSpdlogLevelStatic( LogLevel level );

    private:
      void SetDefaults();
      bool ParseJsonConfig( const std::string& json_content );
    };

  }  // namespace logger
}  // namespace rediscc

#endif  // REDISCC_LOGGER_CONFIG_H
