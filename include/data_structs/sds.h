/* SDSLib 2.0 -- A C dynamic strings library
 *
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2) or the Server Side Public License v1 (SSPLv1).
 */

#ifndef __REDISCC_DATA_STRUCTURES_SDS_H
#define __REDISCC_DATA_STRUCTURES_SDS_H

#include "config.h"
#include "jemalloc/jemalloc.h"
#include "xmalloc.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <unistd.h>
#include <vector>

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
#define sds_assert( condition, fmt, ... )                                       \
  do {                                                                          \
    if ( !( condition ) ) {                                                     \
      REDISCC_ERROR( "Assertion failed: " #condition "--" fmt, ##__VA_ARGS__ ); \
      abort();                                                                  \
    }                                                                           \
  } while ( 0 )

template < typename T >
concept IsUnsignedInteger = std::is_same_v< T, uint8_t > || std::is_same_v< T, uint16_t >
                            || std::is_same_v< T, uint32_t > || std::is_same_v< T, uint64_t >;

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-10
 * @brief
 *
 * @note 原本Redis中的SDS是通过获取buf_的指针，然后再通过结构体对齐后的内存布局来获取整个结构体；
 * @code {.cc}
 * #define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
 * @endcode
 *
 * 这样做有两个好处：
 *  1.内部使用时，直接可以使用buf_指针，而非整个结构体，这样可以直接使用libc函数
 *  2.隐藏不同字段结构体(C语言实现多态)
 *
 * @tparam SDSHDR_TYPE
 */
template < IsUnsignedInteger SDSHDR_TYPE >
class sds_t {
private:
  SDSHDR_TYPE len_;    ///<
  SDSHDR_TYPE alloc_;  ///<
  uint8_t*    buf_;    ///<

public:
  static inline constexpr SDSHDR_TYPE max_value        = std::numeric_limits< SDSHDR_TYPE >::max();
  static inline constexpr SDSHDR_TYPE min_value        = std::numeric_limits< SDSHDR_TYPE >::min();
  static inline constexpr size_t      SDS_MAX_PREALLOC = 1024 * 1024;

public:
  sds_t() { new_length( "", 0 ); }
  sds_t( const char* init_str ) {
    size_t init_len = ( init_str == nullptr ) ? 0 : strlen( init_str );
    new_length( init_str, init_len );
  }
  sds_t( int64_t value );

  ~sds_t() {
    if ( buf_ ) {
      xfree( buf_ );
      buf_ = nullptr;
    }
  }

  auto operator<=>( const sds_t< SDSHDR_TYPE >& other ) const = default;

  bool operator==( const sds_t< SDSHDR_TYPE >& other ) const {
    return len_ == other.len_ && memcmp( buf_, other.buf_, len_ ) == 0;
  }

  bool operator<( const sds_t< SDSHDR_TYPE >& other ) const {
    return len_ < other.len_ || ( len_ == other.len_ && memcpy( buf_, other.buf_, len_ ) < 0 );
  }

  uint8_t operator[]( size_t n ) {
    sds_assert( n >= 0 && n < len_, "overflow the sds's length({})", len_ );
    return buf_[ n ];
  }

public:
  const uint8_t* data() const { return buf_; }

  constexpr size_t length() const { return len_; }

  constexpr size_t get_avail() const {
    size_t avail = alloc_ - len_ - 1;
    sds_assert( avail >= 0, "maybe alloc error! avail bytes equal {}", avail );
    return avail;
  }

  constexpr size_t get_alloc() const { return alloc_; }

public:
  void new_length( const char* init_str, size_t init_len ) {
    buf_ = __new_length( init_str, init_len, false );
  }

  void try_new_length( const char* init_str, size_t init_len ) {
    buf_ = __new_length( init_str, init_len, true );
  }

  sds_t duplicate() { return sds_t( (char*)buf_ ); }

  void resize( size_t size, [[maybe_unused]] bool would_regrow = false ) {
    if ( size == alloc_ ) {
      return;
    }

    ///< 为了记录原始sds数据的`\0`位置
    ///< 如果`size`小于`len_`，则`len_`会被截断
    size_t len = size < len_ ? size : len_;

    bool   use_realloc = size < max_value;
    size_t new_len     = size + 1;  ///< 为`\0`预留空间

    if ( use_realloc ) {
      /**
       * `je_nallocx`会返回实际分配的内存大小，如果`je_nallocx`返回的大小和`xmalloc_size`返回的大小不一致
       * 说明内存分配器可能会重新分配内存。
       * 此时需要调用`xrealloc`重新分配内存，`xrealloc`会返回新的内存地址。并且旧地址会在内部被回收。
       */
      if ( !( je_nallocx( new_len, 0 ) == xmalloc_size( buf_ ) ) ) {
        buf_ = (uint8_t*)xrealloc( buf_, new_len );
        sds_assert( buf_ != nullptr, "xrealloc failed!" );
      }
    } else {
      /**
       * Redis本身支持在`resize`时创建一个新的类型，但是这里不支持。
       * 因为这是对于自身的操作，而不是对于其他对象的操作，如果返回一个新类型的对象也比较奇怪。
       */
      REDISCC_ERROR( "size is too large!" );
      abort();
    }

    buf_[ len ] = 0;
    len_        = len;
    alloc_      = new_len;
  }

  void update_len() { len_ = strlen( (char*)buf_ ); }

  void remove_free_space( bool would_regrow = false ) { resize( len_, would_regrow ); }

  void make_room( size_t add_len ) { buf_ = __make_room( add_len, true ); }

  void make_room_no_greedy( size_t add_len ) { buf_ = __make_room( add_len, false ); }

  void incr_len( ssize_t incr ) {
    sds_assert(
        incr >= 0 && get_avail() >= incr,
        "increase size must be positive or less equal the avail length! incr: {}, avail: {}",
        incr,
        get_avail() );

    len_         += incr;
    buf_[ len_ ]  = '\0';
  }

  void grow_by_zero( size_t len ) {
    if ( len <= length() ) {
      REDISCC_WARN( "grow_by_zero: len is less than length(), no need to grow." );
      return;
    }

    buf_ = make_room( len - length() );
    sds_assert( buf_ != nullptr, "make_room failed!" );

    memset( buf_ + length(), 0, len - length() + 1 );
    len_ = len;
  }

  void catlen( const void* t, size_t len ) {
    make_room( len );
    sds_assert( buf_ != nullptr, "make_room failed!" );
    memcpy( buf_ + length(), t, len );
    len_         += len;
    buf_[ len_ ]  = '\0';
  }

  void    cat( const char* t );
  void    cat( const sds_t< SDSHDR_TYPE >& t );
  void    cpy( const char* t, size_t len );
  void    cpy( const char* t ) { cpy( t, strlen( t ) ); }
  void    trim( const char* cset );
  void    substr( size_t start, size_t len );
  void    subrange( ssize_t start, ssize_t end );
  int32_t compare( const sds_t< SDSHDR_TYPE >& t );

  std::vector< sds_t< SDSHDR_TYPE > >
       split( const char* s, ssize_t len, const char* sep, size_t sep_len );
  void to_lower();
  void to_upper();
  void cat_repr( const char* p, size_t len );

  void clear() {
    len_      = 0;
    buf_[ 0 ] = '\0';
  }

private:
  uint8_t* __new_length( const char* init_str, size_t init_len, bool try_malloc ) {
    uint8_t* ret_s;
    size_t   usable;

    sds_assert( init_len + 1 >= init_len,
                "string length must be less than size_t({0})",
                std::numeric_limits< size_t >::max() );

    ret_s = try_malloc ? (uint8_t*)try_xmalloc_usable( init_len + 1, &usable )
                       : (uint8_t*)xmalloc_usable( init_len + 1, &usable );

    sds_assert( ret_s != nullptr, "sds malloc error!" );

    if ( !strcmp( init_str, "NOINIT" ) ) {
      // TODO DO NOTHING
    } else if ( !init_str ) {
      memset( ret_s, 0, init_len + 1 );
    }

    usable = usable - 1;
    if ( usable > max_value ) {
      usable = max_value;
    }

    len_   = init_len;
    alloc_ = usable;

    if ( init_len && init_str ) {
      memcpy( ret_s, init_str, init_len );
    }
    ret_s[ init_len ] = '\0';

    return ret_s;
  }

  uint8_t* __make_room( size_t add_len, bool greedy ) {
    if ( get_avail() >= add_len ) {
      return buf_;
    }

    size_t usable  = 0;
    size_t new_len = len_ + add_len;

    if ( greedy ) {
      if ( new_len < SDS_MAX_PREALLOC )
        new_len *= 2;
      else
        new_len += SDS_MAX_PREALLOC;
    }

    sds_assert( new_len + 1 >= new_len, "__make_room() failed: overflow" );

    buf_ = (uint8_t*)xrealloc_usable( buf_, new_len + 1, &usable );
    sds_assert( buf_ != nullptr, "__make_room() failed: xrealloc_usable()" );
    sds_assert( usable <= max_value, "__make_room() usable > max_value({})", max_value );

    alloc_ = usable;
    return buf_;
  }
};

using sds_8t  = sds_t< uint8_t >;
using sds_16t = sds_t< uint16_t >;
using sds_32t = sds_t< uint32_t >;
using sds_64t = sds_t< uint64_t >;

#endif  //! __REDISCC_DATA_STRUCTURES_SDS_H
