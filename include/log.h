#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

const std::string CONSOLE_PATTERN = "[%Y-%m-%d %H:%M:%S] [%^%l%$] [%t] %v";
const std::string FILE_PATTERN    = "[%Y-%m-%d %H:%M:%S] [%l] [%t] %v";

void InitRedisCCLogger() {
  try {
    auto console_sink = std::make_shared< spdlog::sinks::stdout_color_sink_mt >();
    console_sink->set_pattern( CONSOLE_PATTERN );

    auto file_sink = std::make_shared< spdlog::sinks::rotating_file_sink_mt >(
        "logs/rediscc.log", 1024 * 1024 * 10, 3 );
    file_sink->set_pattern( FILE_PATTERN );

    std::vector< spdlog::sink_ptr > sinks { console_sink, file_sink };
    auto logger = std::make_shared< spdlog::logger >( "REDISCC", sinks.begin(), sinks.end() );

    spdlog::register_logger( logger );
    logger->set_level( spdlog::level::info );

    // 设置控制台颜色
    console_sink->set_color( spdlog::level::info, "\033[32m" );
    console_sink->set_color( spdlog::level::err, "\033[33m" );
    console_sink->set_color( spdlog::level::warn, "\033[31m" );
  } catch ( const spdlog::spdlog_ex& ex ) {
    // 异常处理
    // TODO
  }
}

// 常规日志宏
#define REDISCC_INFO( ... )  SPDLOG_LOGGER_INFO( spdlog::get( "REDISCC" ), __VA_ARGS__ )
#define REDISCC_ERROR( ... ) SPDLOG_LOGGER_ERROR( spdlog::get( "REDISCC" ), __VA_ARGS__ )
#define REDISCC_WARN( ... )  SPDLOG_LOGGER_WARN( spdlog::get( "REDISCC" ), __VA_ARGS__ )
// 调试日志宏
#ifdef _DEBUG
#  define REDISCC_DEBUG( ... )                                                                   \
    SPDLOG_LOGGER_DEBUG(                                                                         \
        spdlog::get( "REDISCC" ), "[{}:{}] {}", __FILE__, __LINE__, fmt::format( __VA_ARGS__ ) )
#else
#  define REDISCC_DEBUG( ... )
#endif
