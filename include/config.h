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

#  endif
#endif

#endif  //! REDISCC_CONFIG_H
