#include <atomic>
#include <atomicvar.h>
#include <config.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <xmalloc.h>

#ifdef USE_JEMALLOC
#  define malloc( size )              je_malloc( size )
#  define calloc( count, size )       je_calloc( count, size )
#  define realloc( ptr, size )        je_realloc( ptr, size )
#  define free( ptr )                 je_free( ptr )
#  define mallocx( size, flags )      je_mallocx( size, flags )
#  define rallocx( ptr, size, flags ) je_rallocx( ptr, size, flags )
#  define dallocx( ptr, flags )       je_dallocx( ptr, flags )
#else
#  error "only support jemalloc"
#endif

#ifdef HAVE_MALLOC_SIZE
#  define PREFIX_SIZE ( 0 )
#else
#  error "must be have malloc size"
#endif  //!

static const constexpr int64_t MAX_THREADS        = 16;
static const constexpr int64_t THREAD_MASK        = MAX_THREADS - 1;
static std::atomic_int64_t     num_active_threads = 0;
static __thread long           my_thread_index    = -1;

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 用于记录每个线程的内存使用情况的结构体。
 *
 * 该结构体包含一个原子变量 `used_memory`，用于记录当前线程的内存使用量，
 * 以及一个填充数组 `padding`，用于确保结构体的大小为缓存行大小的整数倍，
 * 避免伪共享问题。
 */
struct used_memory_entry {
  std::atomic_int64_t used_memory;                         ///< 当前线程的内存使用量

  int64_t padding[ CACHE_LINE_SIZE - sizeof( int64_t ) ];  ///< 填充数组
                                                           ///< 确保结构体大小为缓存行大小的整数倍
};

[[gnu::aligned( CACHE_LINE_SIZE )]]
static used_memory_entry used_memory[ MAX_THREADS ];  ///< 用于存储每个线程的内存使用情况的数组

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 计算内存分配的最小大小。
 *
 * 该函数用于确保分配的内存大小不会小于0。
 *
 * @param [in] x 请求分配的内存大小
 * @return constexpr size_t 返回实际分配的内存大小
 */
static inline constexpr size_t malloc_min_size( size_t x ) { return x > 0 ? x : sizeof( long ); }

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 初始化当前线程的索引。
 *
 * 该函数用于为当前线程分配一个唯一的索引，并将其存储在 `my_thread_index` 中。
 * 如果当前线程的索引尚未初始化，则通过原子操作递增 `num_active_threads` 来分配索引。
 */
static inline void init_my_thread_index( void ) {
  if ( unlikely( my_thread_index == -1 ) ) {
    atomic_get_inc( num_active_threads, my_thread_index, 1 );
    my_thread_index &= THREAD_MASK;
  }
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 更新内存分配统计信息。
 *
 * 该函数用于在分配内存时更新当前线程的内存使用量。
 *
 * @param [in] num 分配的内存大小
 */
static void update_xmalloc_stat_alloc( int64_t num ) {
  init_my_thread_index();
  atomic_inc( used_memory[ my_thread_index ].used_memory, num );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 更新内存释放统计信息。
 *
 * 该函数用于在释放内存时更新当前线程的内存使用量。
 *
 * @param [in] num 释放的内存大小
 */
static void update_xmalloc_stat_free( int64_t num ) {
  init_my_thread_index();
  atomic_dec( used_memory[ my_thread_index ].used_memory, num );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 默认的内存分配失败处理函数。
 *
 * 该函数在内存分配失败时被调用，输出错误信息并终止程序。
 *
 * @param [in] size 请求分配的内存大小
 */
static void xmalloc_default_oom( size_t size ) {
  REDISCC_ERROR( "xmalloc: Out of memory trying to allocate {0} bytes", size );
  fflush( stderr );
  abort();
}

static std::function< void( size_t ) > xmalloc_oom_handler
    = xmalloc_default_oom;  ///< 内存分配失败处理函数

                            /*************************************
                             *          XMALLOC IMPL
                             ************************************/
/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 尝试分配内存并返回可用的内存大小。
 *
 * 该函数用于分配指定大小的内存，并更新内存使用统计信息。
 * 如果分配成功，返回分配的内存指针，并可选地返回实际可用的内存大小。
 *
 * @param [in] size 请求分配的内存大小
 * @param [out] usable 可选参数，用于返回实际可用的内存大小
 * @return void* 返回分配的内存指针，如果分配失败则返回 `nullptr`
 */
static inline void* try_xmalloc_usable_internal( size_t size, size_t* usable ) {
  if ( size >= SIZE_MAX / 2 ) return nullptr;
  void* ptr = malloc( malloc_min_size( size ) + PREFIX_SIZE );

  if ( !ptr ) return nullptr;
  size = xmalloc_size( ptr );
  update_xmalloc_stat_alloc( size );
  if ( usable ) *usable = size;

  return ptr;
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 尝试分配内存并返回可用的内存大小。
 *
 * 该函数用于分配指定大小的内存，并返回实际可用的内存大小。
 *
 * @param [in] size 请求分配的内存大小
 * @param [out] usable 用于返回实际可用的内存大小
 * @return void* 返回分配的内存指针，如果分配失败则返回 `nullptr`
 */
void* try_xmalloc_usable( size_t size, size_t* usable ) {
  size_t usable_size = 0;
  void*  ptr         = try_xmalloc_usable_internal( size, &usable_size );
  ptr                = extend_to_usable( ptr, usable_size );
  if ( usable ) *usable = usable_size;
  return ptr;
}

void* try_xmalloc( size_t size ) {
  void* ptr = try_xmalloc_usable_internal( size, nullptr );
  return ptr;
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 分配指定大小的内存并返回实际可用的内存大小。
 *
 * 该函数用于分配指定大小的内存，并在分配失败时调用内存分配失败处理函数。
 * 如果分配成功，返回分配的内存指针，并可选地返回实际可用的内存大小。
 *
 * @param [in] size 请求分配的内存大小
 * @param [out] usable 可选参数，用于返回实际可用的内存大小
 * @return void* 返回分配的内存指针，如果分配失败则调用 `xmalloc_oom_handler`
 */
void* xmalloc_usable( size_t size, size_t* usable ) {
  size_t usable_size = 0;
  void*  ptr         = try_xmalloc_usable_internal( size, &usable_size );
  if ( !ptr ) xmalloc_oom_handler( size );
  ptr = extend_to_usable( ptr, usable_size );
  if ( usable ) *usable = usable_size;
  return ptr;
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 分配指定大小的内存。
 *
 * 该函数用于分配指定大小的内存，并在分配失败时调用内存分配失败处理函数。
 *
 * @param [in] size 请求分配的内存大小
 * @return void* 返回分配的内存指针，如果分配失败则调用 `xmalloc_oom_handler`
 */
void* xmalloc( size_t size ) {
  void* ptr = try_xmalloc_usable_internal( size, nullptr );
  if ( !ptr ) xmalloc_oom_handler( size );
  return ptr;
}

/*************************************
 *          XCALLOC IMPL
 ************************************/
static inline void* try_xcalloc_usable_internal( size_t size, size_t* usable ) {
  if ( size >= SIZE_MAX / 2 ) return nullptr;
  void* ptr = calloc( 1, malloc_min_size( size ) + PREFIX_SIZE );

  if ( !ptr ) return nullptr;
  size = xmalloc_size( ptr );
  update_xmalloc_stat_alloc( size );
  if ( usable ) *usable = size;
  return ptr;
}

void* try_xcalloc_usable( size_t size, size_t* usable ) {
  size_t usable_size = 0;
  void*  ptr         = try_xcalloc_usable_internal( size, &usable_size );
  ptr                = extend_to_usable( ptr, usable_size );
  if ( usable ) *usable = usable_size;
  return ptr;
}

void* try_xcalloc( size_t size ) {
  void* ptr = try_xcalloc_usable_internal( size, nullptr );
  if ( !ptr ) xmalloc_oom_handler( size );
  return ptr;
}

void* xcalloc_usable( size_t size, size_t* usable ) {
  size_t usable_size = 0;
  void*  ptr         = try_xcalloc_usable_internal( size, &usable_size );
  ptr                = extend_to_usable( ptr, usable_size );
  if ( usable ) *usable = usable_size;
  return ptr;
}

void* xcalloc_num( size_t num, size_t size ) {
  if ( ( size == 0 ) || ( num > SIZE_MAX / size ) ) {
    xmalloc_oom_handler( SIZE_MAX );
  }
  void* ptr = try_xcalloc_usable_internal( size, nullptr );
  if ( !ptr ) xmalloc_oom_handler( num * size );
  return ptr;
}

void* xcalloc( size_t size ) {
  void* ptr = try_xcalloc_usable_internal( size, nullptr );
  return ptr;
}

/*************************************
 *          XREALLOC IMPL
 ************************************/
/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 尝试重新分配内存并返回可用的内存大小。
 *
 * 该函数用于重新分配指定大小的内存，并更新内存使用统计信息。
 * 如果重新分配成功，返回新的内存指针，并可选地返回实际可用的内存大小。
 *
 * @param [in] ptr 原始内存指针
 * @param [in] size 请求重新分配的内存大小
 * @param [out] usable 可选参数，用于返回实际可用的内存大小
 * @return void* 返回重新分配的内存指针，如果重新分配失败则返回 `nullptr`
 */
static inline void* try_xrealloc_usable_internal( void* ptr, size_t size, size_t* usable ) {
  size_t old_size;
  void*  new_ptr;

  if ( size == 0 && ptr != nullptr ) {
    xfree( ptr );
    if ( usable ) *usable = 0;
    return nullptr;
  }

  if ( ptr == nullptr ) return try_xmalloc_usable( size, usable );

  if ( size >= SIZE_MAX / 2 ) {
    xfree( ptr );
    if ( usable ) *usable = 0;
    return nullptr;
  }

  old_size = xmalloc_size( ptr );
  /**
   * `je_realloc`在分配内存时，通常会按 2 的幂次倍数分配
   *    - 对于 小对象（具体大小取决于 Jemalloc 的配置，通常是 1 KB
   * 或以下），内存大小会被对齐到特定的粒度（如 8 字节或 16 字节）
   *    - 对于 中等大小的对象，内存大小会对齐到更大的粒度，通常是 2 的幂次倍数（例如 32 字节、64
   * 字节、128 字节）
   *    - 对于 大对象（大于某个阈值，例如 1 MB），Jemalloc 会使用页粒度分配，粒度通常是 4
   * KB（或者更大的系统页大小）
   */
  new_ptr  = realloc( ptr, size );
  if ( new_ptr == nullptr ) {
    if ( usable ) *usable = 0;
    return nullptr;
  }

  update_xmalloc_stat_free( old_size );
  size = xmalloc_size( new_ptr );
  update_xmalloc_stat_alloc( size );
  if ( usable ) *usable = size;

  return new_ptr;
}

void* try_xrealloc_usable( void* ptr, size_t size, size_t* usable ) {
  size_t usable_size = 0;
  ptr                = try_xrealloc_usable_internal( ptr, size, &usable_size );
  ptr                = extend_to_usable( ptr, usable_size );
  if ( usable ) *usable = usable_size;
  return ptr;
}

void* try_xrealloc( void* ptr, size_t size ) {
  ptr = try_xrealloc_usable_internal( ptr, size, nullptr );
  return ptr;
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-13
 * @brief 重新分配指定大小的内存并返回实际可用的内存大小。
 *
 * 该函数用于重新分配指定大小的内存，并在重新分配失败时调用内存分配失败处理函数。
 * 如果重新分配成功，返回新的内存指针，并可选地返回实际可用的内存大小。
 *
 * @param [in] ptr 原始内存指针
 * @param [in] size 请求重新分配的内存大小
 * @param [out] usable 可选参数，用于返回实际可用的内存大小
 * @return void* 返回重新分配的内存指针，如果重新分配失败则调用 `xmalloc_oom_handler`
 */
void* xrealloc_usable( void* ptr, size_t size, size_t* usable ) {
  size_t usable_size = 0;
  ptr                = try_xrealloc_usable( ptr, size, &usable_size );
  if ( !ptr && size != 0 ) xmalloc_oom_handler( size );
  ptr = extend_to_usable( ptr, usable_size );
  if ( usable ) *usable = usable_size;
  return ptr;
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 重新分配指定大小的内存。
 *
 * 该函数用于重新分配指定大小的内存，并在重新分配失败时调用内存分配失败处理函数。
 *
 * @param [in] ptr 原始内存指针
 * @param [in] size 请求重新分配的内存大小
 * @return void* 返回重新分配的内存指针，如果重新分配失败则调用 `xmalloc_oom_handler`
 */
void* xrealloc( void* ptr, size_t size ) {
  ptr = try_xrealloc_usable_internal( ptr, size, nullptr );
  if ( !ptr && size != 0 ) xmalloc_oom_handler( size );
  return ptr;
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 释放指定的内存。
 *
 * 该函数用于释放指定的内存，并更新内存使用统计信息。
 *
 * @param [in] ptr 要释放的内存指针
 */
void xfree( void* ptr ) {
  if ( ptr == nullptr ) return;
  update_xmalloc_stat_free( xmalloc_size( ptr ) );
  free( ptr );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 扩展内存块以使其可用。
 *
 * 该函数用于扩展内存块以使其可用，目前未实现具体功能，直接返回原始指针。
 *
 * @param [in] ptr 原始内存指针
 * @param [in] size 内存块大小
 * @return void* 返回扩展后的内存指针
 */
void* extend_to_usable( void* ptr, [[maybe_unused]] size_t size ) { return ptr; }
