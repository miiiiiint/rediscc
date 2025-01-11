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

#    define REDISCC_INFO  spdlog::info
#    define REDISCC_DEBUG spdlog::debug
#    define REDISCC_WARN  spdlog::warn
#    define REDISCC_ERROR spdlog::error
#    define REDISCC_FATAL spdlog::fatal

#  endif
#endif

#endif  //! REDISCC_CONFIG_H
