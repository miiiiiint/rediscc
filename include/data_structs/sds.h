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
#include <cstddef>
#include <cstdint>
#include <cstdio>

using sds = char*;

enum class sds_type : uint8_t {
  TYPE_5  = 0,
  TYPE_8  = 1,
  TYPE_16 = 2,
  TYPE_32 = 3,
  TYPE_64 = 4,

  TYPE_BITS = 3,
  TYPE_MASK = 7,
};

struct [[gnu::packed]] sdshdr5 {
  unsigned char flags;
  char          buf[];
};

template < typename SDS_TYPE >
struct [[gnu::packed]] sdshdr {
  SDS_TYPE len;
  SDS_TYPE alloc;
  uint8_t  flags;
  char     buf[];
};

using sdshdr8  = sdshdr< uint8_t >;
using sdshdr16 = sdshdr< uint16_t >;
using sdshdr32 = sdshdr< uint32_t >;
using sdshdr64 = sdshdr< uint64_t >;

constexpr sds_type get_sds_type( const sds s ) {
  return static_cast< sds_type >( s[ -1 ] & (int)sds_type::TYPE_MASK );
}

template < typename SDS_TYPE >
SDS_TYPE* get_hdr_ptr( const sds s ) {
  return reinterpret_cast< SDS_TYPE* >( s - offsetof( SDS_TYPE, buf ) );
}

template < typename SDS_TYPE >
constexpr size_t __sdslen( const sds s ) {
  return get_hdr_ptr< SDS_TYPE >( s )->len;
}

template < typename SDS_TYPE >
constexpr size_t __sdsalloc( const sds s ) {
  return get_hdr_ptr< SDS_TYPE >( s )->alloc;
}

constexpr static inline size_t sdslen( const sds s ) {
  switch ( get_sds_type( s ) ) {
  case sds_type::TYPE_5 : REDISCC_WARN( "sdshdr5 should not to be used" ); return 0;
  case sds_type::TYPE_8 : return __sdslen< sdshdr8 >( s );
  case sds_type::TYPE_16: return __sdslen< sdshdr16 >( s );
  case sds_type::TYPE_32: return __sdslen< sdshdr32 >( s );
  case sds_type::TYPE_64: return __sdslen< sdshdr64 >( s );
  default               : return 0;
  }
}

constexpr static inline size_t sdsavail( const sds s ) {
  switch ( get_sds_type( s ) ) {
  case sds_type::TYPE_5 : REDISCC_WARN( "sdshdr5 should not to be used" ); return 0;
  case sds_type::TYPE_8 : return __sdsalloc< sdshdr8 >( s ) - __sdslen< sdshdr8 >( s );
  case sds_type::TYPE_16: return __sdsalloc< sdshdr16 >( s ) - __sdslen< sdshdr16 >( s );
  case sds_type::TYPE_32: return __sdsalloc< sdshdr32 >( s ) - __sdslen< sdshdr32 >( s );
  case sds_type::TYPE_64: return __sdsalloc< sdshdr64 >( s ) - __sdslen< sdshdr64 >( s );
  default               : return 0;
  }
}

constexpr static inline void sdssetlen( sds s, size_t new_len ) {
  switch ( get_sds_type( s ) ) {
  case sds_type::TYPE_5 : REDISCC_WARN( "sdshdr5 should not to be used" ); break;
  case sds_type::TYPE_8 : get_hdr_ptr< sdshdr8 >( s )->len = new_len; break;
  case sds_type::TYPE_16: get_hdr_ptr< sdshdr16 >( s )->len = new_len; break;
  case sds_type::TYPE_32: get_hdr_ptr< sdshdr32 >( s )->len = new_len; break;
  case sds_type::TYPE_64: get_hdr_ptr< sdshdr64 >( s )->len = new_len; break;
  default               : break;
  }
}

constexpr static inline void sdsinclen( sds s, size_t inc ) {
  switch ( get_sds_type( s ) ) {
  case sds_type::TYPE_5 : REDISCC_WARN( "sdshdr5 should not to be used" ); break;
  case sds_type::TYPE_8 : get_hdr_ptr< sdshdr8 >( s )->len += inc; break;
  case sds_type::TYPE_16: get_hdr_ptr< sdshdr16 >( s )->len += inc; break;
  case sds_type::TYPE_32: get_hdr_ptr< sdshdr32 >( s )->len += inc; break;
  case sds_type::TYPE_64: get_hdr_ptr< sdshdr64 >( s )->len += inc; break;
  default               : break;
  }
}

constexpr static inline size_t sdsalloc( const sds s ) {
  switch ( get_sds_type( s ) ) {
  case sds_type::TYPE_5 : REDISCC_WARN( "sdshdr5 should not to be used" ); return 0;
  case sds_type::TYPE_8 : return __sdsalloc< sdshdr8 >( s );
  case sds_type::TYPE_16: return __sdsalloc< sdshdr16 >( s );
  case sds_type::TYPE_32: return __sdsalloc< sdshdr32 >( s );
  case sds_type::TYPE_64: return __sdsalloc< sdshdr64 >( s );
  default               : return 0;
  }
}

constexpr static inline void sdssetalloc( sds s, size_t new_len ) {
  switch ( get_sds_type( s ) ) {
  case sds_type::TYPE_5 : REDISCC_WARN( "sdshdr5 should not to be used" ); break;
  case sds_type::TYPE_8 : get_hdr_ptr< sdshdr8 >( s )->alloc = new_len; break;
  case sds_type::TYPE_16: get_hdr_ptr< sdshdr16 >( s )->alloc = new_len; break;
  case sds_type::TYPE_32: get_hdr_ptr< sdshdr32 >( s )->alloc = new_len; break;
  case sds_type::TYPE_64: get_hdr_ptr< sdshdr64 >( s )->alloc = new_len; break;
  default               : break;
  }
}

void                 sdsfree( sds s );
std::optional< sds > sdsnew( const char* init );
std::optional< sds > sdsnewlen( const void* init, size_t initlen );
std::optional< sds > sdstrynewlen( const void* init, size_t initlen );
sds                  sdsempty( void );
sds                  sdsdup( const sds s );
sds                  sdsgrowzero( sds s, size_t len );
sds                  sdscat( sds s, const void* t, size_t len );
sds                  sdscat( sds s, const char* t );
sds                  sdscat( sds s, const sds t );
sds                  sdscat( sds s, char const* fmt, ... );
sds                  sdscpy( sds s, const char* t, size_t len );
sds                  sdscpy( sds s, const char* t );
sds                  sdscatvprintf( sds s, const char* fmt, va_list ap );
[[gnu::format( printf, 2, 3 )]]
sds    sdscatprintf( sds s, const char* fmt, ... );
sds    sdstrim( sds s, const char* cset );
void   sdssubstr( sds s, size_t start, size_t len );
void   sdsrange( sds s, ssize_t start, ssize_t end );
void   sdsupdatelen( sds s );
void   sdsclear( sds s );
int8_t sdscmp( const sds s1, const sds s2 );
sds*   sdssplitlen( const char* s, ssize_t len, const char* sep, int seplen, int* count );
void   sdsfreesplitres( sds* tokens, int count );
void   sdstolower( sds s );
void   sdstoupper( sds s );
sds    sdsfromlonglong( long long value );
sds    sdscatrepr( sds s, const char* p, size_t len );
sds*   sdssplitargs( const char* line, int* argc );
sds    sdsmapchars( sds s, const char* from, const char* to, size_t setlen );
sds    sdsjoin( char** argv, int argc, char* sep );
sds    sdsjoinsds( sds* argv, int argc, const char* sep, size_t seplen );
int    sdsneedsrepr( const sds s );

sds    sdsMakeRoomFor( sds s, size_t addlen );
sds    sdsMakeRoomForNonGreedy( sds s, size_t addlen );
void   sdsIncrLen( sds s, ssize_t incr );
sds    sdsRemoveFreeSpace( sds s, int would_regrow );
sds    sdsResize( sds s, size_t size, int would_regrow );
size_t sdsAllocSize( sds s );
void*  sdsAllocPtr( sds s );

#endif  //! __REDISCC_DATA_STRUCTURES_SDS_H
