#ifndef REDISCC_ALLOCATOR_XMALLOC_H
#define REDISCC_ALLOCATOR_XMALLOC_H

#include <cstddef>

#ifdef USE_JEMALLOC
#  ifdef __has_include
#    if __has_include( <jemalloc/jemalloc.h> )
#      include <jemalloc/jemalloc.h>
#      define HAVE_MALLOC_SIZE  1
#      define xmalloc_size( p ) je_malloc_usable_size( p )
#    else
#      error "have no Jemalloc"
#    endif
#  endif
#else
#  error "only support Jemalloc"
#endif  // ！USE_JEMALLOC

#define XMALLOC_HELPER_1            [[gnu::malloc, gnu::alloc_size( 1 ), gnu::noinline]]
#define XMALLOC_HELPER_2            [[gnu::malloc, gnu::alloc_size( 2 ), gnu::noinline]]
#define XMALLOC_HELPER_1AND2        [[gnu::malloc, gnu::alloc_size( 1, 2 ), gnu::noinline]]
#define XMALLOC_NOT_MALLOC_HELPER_2 [[gnu::alloc_size( 2 ), gnu::noinline]]
#define XMALLOC_EXTERN              extern

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief
 *
 * @param [in] size
 * @return void*
 */
XMALLOC_HELPER_1 XMALLOC_EXTERN void* xmalloc( size_t size );

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief
 *
 * @param [out] ptr
 * @param [in] size
 * @return void*
 */
XMALLOC_HELPER_2 XMALLOC_EXTERN void* xrealloc( void* ptr, size_t size );

XMALLOC_EXTERN void  xfree( void* ptr );
XMALLOC_EXTERN void* xmalloc_usable( size_t size, size_t* usable );
XMALLOC_EXTERN void* xrealloc_usable( void* ptr, size_t size, size_t* usable );
XMALLOC_EXTERN void* try_xmalloc_usable( size_t size, size_t* usable );

XMALLOC_NOT_MALLOC_HELPER_2 XMALLOC_EXTERN void* extend_to_usable( void* ptr, size_t size );

#endif  //! REDISCC_ALLOCATOR_XMALLOC_H
