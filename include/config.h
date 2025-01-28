#ifndef REDISCC_CONFIG_H
#define REDISCC_CONFIG_H

#if __GNUC__ >= 3
#  define likely( x )   __builtin_expect( !!( x ), 1 )
#  define unlikely( x ) __builtin_expect( !!( x ), 0 )
#else
#  define likely( x )   ( x )
#  define unlikely( x ) ( x )
#endif

#ifndef CACHE_LINE_SIZE
#  define CACHE_LINE_SIZE 64
#else
#  error "only support CACHE_LINE_SIZE = 64"
#endif

#ifdef USE_SPDLOG
#  if __has_include( <spdlog/spdlog.h> )
#    include <spdlog/spdlog.h>

#    define REDISCC_INFO( ... )  spdlog::info( __VA_ARGS__ )
#    define REDISCC_DEBUG( ... ) spdlog::debug( __VA_ARGS__ )
#    define REDISCC_WARN( ... )  spdlog::warn( __VA_ARGS__ )
#    define REDISCC_ERROR( ... ) spdlog::error( __VA_ARGS__ )

/**
 * @brief 用于在条件不满足时触发错误并终止程序
 *
 * @param condition 条件表达式，如果为false，则触发错误
 * @param fmt 格式化字符串，用于输出错误信息
 * @param ... 可变参数，用于格式化字符串中的占位符
 *
 * @note 该函数主要用于在调试或开发阶段捕获潜在的错误条件，并在条件不满足时立即终止程序。
 *       使用REDISCC_ERROR宏记录错误信息，并通过abort()终止程序。
 */
#    define rediscc_assert( condition, fmt, ... )                                   \
      do {                                                                          \
        if ( !( condition ) ) {                                                     \
          REDISCC_ERROR( "Assertion failed: " #condition "--" fmt, ##__VA_ARGS__ ); \
          abort();                                                                  \
        }                                                                           \
      } while ( 0 )

#  endif
#endif

#endif  //! REDISCC_CONFIG_H
