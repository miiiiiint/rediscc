/* See endianconv.c top comments for more information
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2011-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2) or the Server Side Public License v1 (SSPLv1).
 */

#ifndef REDISCC_UTIL_ENDIANCONV_H
#define REDISCC_UTIL_ENDIANCONV_H

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#if __has_include( <endian.h> )
#  include <endian.h>
#endif

template < typename T >
constexpr void memrev_ifbe( void* ptr ) {
  if constexpr ( std::endian::native == std::endian::big ) {
    static_assert( std::is_integral_v< T >, "Only integral types are supported" );
    T* data = reinterpret_cast< T* >( ptr );

    for ( size_t i = 0; i < sizeof( data ) / 2; i++ ) {
      uint8_t* byte_ptr = reinterpret_cast< uint8_t* >( data );
      std::swap( byte_ptr[ i ], byte_ptr[ sizeof( data ) - i - 1 ] );
    }
  }
}

template < typename T >
constexpr T intrev_ifbe( T val ) {
  if constexpr ( std::endian::native == std::endian::big ) {
    static_assert( std::is_integral_v< T >, "Only integral types are supported" );

    if constexpr ( sizeof( T ) == 2 ) {
      return static_cast< T >( ( ( val & 0x00FF ) << 8 ) | ( ( val & 0xFF00 ) >> 8 ) );
    } else if constexpr ( sizeof( T ) == 4 ) {
      return static_cast< T >( ( ( val & 0x000000FF ) << 24 ) | ( ( val & 0x0000FF00 ) << 8 )
                               | ( ( val & 0x00FF0000 ) >> 8 ) | ( ( val & 0xFF000000 ) >> 24 ) );
    } else if constexpr ( sizeof( T ) == 8 ) {
      return static_cast< T >(
          ( ( val & 0x00000000000000FFULL ) << 56 ) | ( ( val & 0x000000000000FF00ULL ) << 40 )
          | ( ( val & 0x0000000000FF0000ULL ) << 24 ) | ( ( val & 0x00000000FF000000ULL ) << 8 )
          | ( ( val & 0x000000FF00000000ULL ) >> 8 ) | ( ( val & 0x0000FF0000000000ULL ) >> 24 )
          | ( ( val & 0x00FF000000000000ULL ) >> 40 ) | ( ( val & 0xFF00000000000000ULL ) >> 56 ) );
    } else {
      static_assert( sizeof( T ) == 2 || sizeof( T ) == 4 || sizeof( T ) == 8,
                     "Unsupported type size for byte reversal" );
    }
  } else {
    return val;
  }
}

#endif  //! REDISCC_UTIL_ENDIANCONV_H
