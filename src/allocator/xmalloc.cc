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
#define malloc( size )              je_malloc( size )
#define calloc( count, size )       je_calloc( count, size )
#define realloc( ptr, size )        je_realloc( ptr, size )
#define free( ptr )                 je_free( ptr )
#define mallocx( size, flags )      je_mallocx( size, flags )
#define rallocx( ptr, size, flags ) je_rallocx( ptr, size, flags )
#define dallocx( ptr, flags )       je_dallocx( ptr, flags )
#else
#error "only support jemalloc"
#endif

#ifdef HAVE_MALLOC_SIZE
#define PREFIX_SIZE ( 0 )
#else
#error "must be have malloc size"
#endif //!

static const constexpr int64_t MAX_THREADS        = 16;
static const constexpr int64_t THREAD_MASK        = MAX_THREADS - 1;
static std::atomic_int64_t     num_active_threads = 0;
static __thread long           my_thread_index    = -1;

struct used_memory_entry {
  std::atomic_int64_t used_memory;
  int64_t             padding[ CACHE_LINE_SIZE - sizeof( int64_t ) ];
};

[[gnu::aligned( CACHE_LINE_SIZE )]]
static used_memory_entry used_memory[ MAX_THREADS ];

static inline constexpr size_t malloc_min_size( size_t x ) {
  return x > 0 ? x : sizeof( long );
}

static inline void init_my_thread_index( void ) {
  if ( unlikely( my_thread_index == -1 ) ) {
    atomic_get_inc( num_active_threads, my_thread_index, 1 );
    my_thread_index &= THREAD_MASK;
  }
}

static void update_xmalloc_stat_alloc( int64_t num ) {
  init_my_thread_index();
  atomic_inc( used_memory[ my_thread_index ].used_memory, num );
}

static void update_xmalloc_stat_free( int64_t num ) {
  init_my_thread_index();
  atomic_dec( used_memory[ my_thread_index ].used_memory, num );
}

static inline void* try_xmalloc_usable_internal( size_t size, size_t* usable ) {
  if ( size >= SIZE_MAX / 2 ) return nullptr;
  void* ptr = malloc( malloc_min_size( size ) + PREFIX_SIZE );

  if ( !ptr ) return nullptr;
  size = xmalloc_size( ptr );
  update_xmalloc_stat_alloc( size );
  if ( usable ) *usable = size;

  return ptr;
}

static inline void* try_xrealloc_usable_internal( void* ptr, size_t size,
                                                  size_t* usable ) {
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
  new_ptr  = realloc( ptr, size );
  if ( new_ptr == nullptr ) {
    if ( usable ) *usable = 0;
    return nullptr;
  }

  update_xmalloc_stat_free( old_size );
  size = xmalloc_size( ptr );
  update_xmalloc_stat_alloc( size );
  if ( usable ) *usable = size;

  return new_ptr;
}

static void xmalloc_default_oom( size_t size ) {
  REDISCC_ERROR( "xmalloc: Out of memory trying to allocate {0} bytes", size );
  fflush( stderr );
  abort();
}

static std::function< void( size_t ) > xmalloc_oom_handler =
  xmalloc_default_oom;

void* xmalloc( size_t size ) {
  void* ptr = try_xmalloc_usable_internal( size, nullptr );
  if ( !ptr ) xmalloc_oom_handler( size );
  return ptr;
}

void* xrealloc( void* ptr, size_t size ) {
  ptr = try_xrealloc_usable_internal( ptr, size, nullptr );
  if ( !ptr && size != 0 ) xmalloc_oom_handler( size );
  return ptr;
}

void xfree( void* ptr ) {
  if ( ptr == nullptr ) return;
  update_xmalloc_stat_free( xmalloc_size( ptr ) );
  free( ptr );
}

void* try_xmalloc_usable( size_t size, size_t* usable ) {
  size_t usable_size = 0;
  void*  ptr         = try_xmalloc_usable_internal( size, &usable_size );
  ptr                = extend_to_usable( ptr, usable_size );
  if ( usable ) *usable = usable_size;
  return ptr;
}

void* extend_to_usable( void* ptr, [[maybe_unused]] size_t size ) {
  return ptr;
}
