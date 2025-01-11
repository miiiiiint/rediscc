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
#include "xmalloc.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>

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
  sds_t() { new_length( "", 0 ); }
  sds_t( const char* init_str ) {
    size_t init_len = ( init_str == nullptr ) ? 0 : strlen( init_str );
    new_length( init_str, init_len );
  }
  ~sds_t() {
    if ( buf_ ) {
      xfree( buf_ );
      buf_ = nullptr;
    }
  }

  uint8_t operator[]( size_t n ) {
    if ( n < 0 && n > len_ ) {
      REDISCC_ERROR( "overflow the sds's length({})", len_ );
      abort();
    }
    return buf_[ n ];
  }

public:
  const uint8_t* data() const { return buf_; }

  size_t length() const { return len_; }

  size_t get_avail() const {
    size_t avail = alloc_ - len_ - 1;
    if ( avail < 0 ) {
      REDISCC_ERROR( "maybe alloc error! avail bytes equal {}", avail );
      abort();
    }
    return avail;
  }

  size_t get_alloc() const { return alloc_; }

public:
  void new_length( const char* init_str, size_t init_len ) {
    buf_ = __new_length( init_str, init_len, false );
  }

  void try_new_length( const char* init_str, size_t init_len ) {
    buf_ = __new_length( init_str, init_len, true );
  }

  sds_t duplicate() { return sds_t( (char*)buf_ ); }

private:
  uint8_t* __new_length( const char* init_str, size_t init_len, bool try_malloc ) {
    uint8_t* ret_s;
    size_t   usable;

    if ( init_len + 1 < init_len ) {
      REDISCC_ERROR( "string length must be less than size_t({0})",
                     std::numeric_limits< size_t >::max() );
      abort();
    }

    ret_s = try_malloc ? (uint8_t*)try_xmalloc_usable( init_len + 1, &usable )
                       : (uint8_t*)xmalloc_usable( init_len + 1, &usable );

    if ( !ret_s ) {
      REDISCC_ERROR( "sds malloc error!" );
      abort();
    }

    if ( !strcmp( init_str, "NOINIT" ) ) {
      // TODO DO NOTHING
    } else if ( !init_str ) {
      memset( ret_s, 0, init_len + 1 );
    }

    usable = usable - 1;
    if ( usable > std::numeric_limits< SDSHDR_TYPE >::max() - 1 ) {
      usable = std::numeric_limits< SDSHDR_TYPE >::max() - 1;
    }

    len_   = init_len;
    alloc_ = usable;

    if ( init_len && init_str ) {
      memcpy( ret_s, init_str, init_len );
    }
    ret_s[ init_len ] = '\0';

    return ret_s;
  }
};

using sds_8t  = sds_t< uint8_t >;
using sds_16t = sds_t< uint16_t >;
using sds_32t = sds_t< uint32_t >;
using sds_64t = sds_t< uint64_t >;

#endif  //! __REDISCC_DATA_STRUCTURES_SDS_H
