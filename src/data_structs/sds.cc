#include "sds.h"
#include "config.h"
#include "xmalloc.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <utility>

constexpr const char* sds_noinit = "SDS_NOINIT";

constexpr static inline std::pair< sds_type, size_t > get_hdr_attr( size_t string_size ) {
  if ( string_size < 1 << 8 ) {
    return { sds_type::TYPE_8, sizeof( sdshdr8 ) };
  } else if ( string_size < 1 << 16 ) {
    return { sds_type::TYPE_16, sizeof( sdshdr16 ) };
  } else if ( string_size < 1ll << 32 ) {
    return { sds_type::TYPE_32, sizeof( sdshdr32 ) };
  } else {
    return { sds_type::TYPE_64, sizeof( sdshdr64 ) };
  }
}

constexpr static inline uint64_t get_type_max( sds_type type ) {
  switch ( type ) {
  case sds_type::TYPE_5 : return 0;
  case sds_type::TYPE_8 : return std::numeric_limits< uint8_t >::max();
  case sds_type::TYPE_16: return std::numeric_limits< uint16_t >::max();
  case sds_type::TYPE_32: return std::numeric_limits< uint32_t >::max();
  case sds_type::TYPE_64: return std::numeric_limits< uint64_t >::max();
  default               : return 0;
  }
}

std::optional< sds > _sdsnewlen( const void* init, size_t initlen, bool try_malloc ) {
  auto [ hdr_type, hdr_len ] = get_hdr_attr( initlen );
  size_t usable              = 0;
  size_t should_alloc_len    = hdr_len + initlen + 1;
  rediscc_assert( should_alloc_len > initlen, "must leave enough space for `hdr + initlen + 1`" );

  void* new_sds = try_malloc ? try_xmalloc_usable( should_alloc_len, &usable )
                             : xmalloc_usable( should_alloc_len, &usable );
  if ( new_sds == nullptr ) {
    REDISCC_WARN( "_sdsnewlen malloc ptr error!" );
    return std::nullopt;
  }

  if ( init == sds_noinit ) {
    init = nullptr;
  } else if ( !init ) {
    memset( new_sds, 0, should_alloc_len );
  }

  usable = usable - hdr_len - 1;
  if ( usable > get_type_max( hdr_type ) ) {
    REDISCC_WARN( "usable greater that type max! usable({}) vs type_max({})",
                  usable,
                  get_type_max( hdr_type ) );
    usable = get_type_max( hdr_type );
  }

  sds buf = static_cast< char* >( new_sds ) + hdr_len;

  // Set the type flag in the byte before the buffer
  uint8_t* flag = ( (uint8_t*)buf ) - 1;
  *flag         = static_cast< uint8_t >( hdr_type );

  // Set length and allocation size
  sdssetlen( buf, initlen );
  sdssetalloc( buf, usable );

  if ( initlen && init ) {
    memcpy( buf, init, initlen );
  }
  buf[ initlen ] = '\0';

  return buf;
}

std::optional< sds > sdsnew( const char* init ) {
  size_t initlen = ( init == nullptr ) ? 0 : strlen( init );
  return sdsnewlen( init, initlen );
}

std::optional< sds > sdsnewlen( const void* init, size_t initlen ) {
  return _sdsnewlen( init, initlen, false );
}

std::optional< sds > sdstrynewlen( const void* init, size_t initlen ) {
  return _sdsnewlen( init, initlen, true );
}

/**
 * @brief Create an empty SDS string
 * @return Empty SDS string, or nullptr on failure
 */
sds sdsempty( void ) {
  auto result = sdsnewlen( "", 0 );
  return result ? *result : nullptr;
}

/**
 * @brief Free an SDS string
 * @param s SDS string to free
 */
void sdsfree( sds s ) {
  if ( s == nullptr ) return;

  sds_type type = get_sds_type( s );
  size_t   hdr_len;

  switch ( type ) {
  case sds_type::TYPE_5 : hdr_len = sizeof( sdshdr5 ); break;
  case sds_type::TYPE_8 : hdr_len = sizeof( sdshdr8 ); break;
  case sds_type::TYPE_16: hdr_len = sizeof( sdshdr16 ); break;
  case sds_type::TYPE_32: hdr_len = sizeof( sdshdr32 ); break;
  case sds_type::TYPE_64: hdr_len = sizeof( sdshdr64 ); break;
  default               : return;  // Invalid type
  }

  void* ptr = s - hdr_len;
  xfree( ptr );
}

/**
 * @brief Make room for additional data in SDS string
 * @param s SDS string
 * @param addlen Additional length needed
 * @return Reallocated SDS string, or nullptr on failure
 */
sds sdsMakeRoomFor( sds s, size_t addlen ) {
  if ( s == nullptr ) return nullptr;

  size_t avail = sdsavail( s );
  if ( avail >= addlen ) return s;  // Already have enough space

  size_t len    = sdslen( s );
  size_t newlen = len + addlen;

  // Growth strategy: double the size if less than 1MB, otherwise add 1MB
  if ( newlen < 1024 * 1024 ) {
    newlen *= 2;
  } else {
    newlen += 1024 * 1024;
  }

  auto [ new_type, new_hdr_len ] = get_hdr_attr( newlen );
  sds_type old_type              = get_sds_type( s );
  size_t   old_hdr_len;

  switch ( old_type ) {
  case sds_type::TYPE_5 : old_hdr_len = sizeof( sdshdr5 ); break;
  case sds_type::TYPE_8 : old_hdr_len = sizeof( sdshdr8 ); break;
  case sds_type::TYPE_16: old_hdr_len = sizeof( sdshdr16 ); break;
  case sds_type::TYPE_32: old_hdr_len = sizeof( sdshdr32 ); break;
  case sds_type::TYPE_64: old_hdr_len = sizeof( sdshdr64 ); break;
  default               : return nullptr;
  }

  void*  old_ptr = s - old_hdr_len;
  size_t usable  = 0;
  void*  new_ptr = xrealloc_usable( old_ptr, new_hdr_len + newlen + 1, &usable );

  if ( new_ptr == nullptr ) return nullptr;

  usable = usable - new_hdr_len - 1;
  if ( usable > get_type_max( new_type ) ) {
    usable = get_type_max( new_type );
  }

  sds new_s = static_cast< char* >( new_ptr ) + new_hdr_len;

  // If header type changed, we need to move data and update header
  if ( new_type != old_type ) {
    memmove( new_s, static_cast< char* >( new_ptr ) + old_hdr_len, len + 1 );
  }

  sdssetlen( new_s, len );
  sdssetalloc( new_s, usable );
  uint8_t* flag = ( (uint8_t*)new_s ) - 1;
  *flag         = static_cast< uint8_t >( new_type );

  return new_s;
}

/**
 * @brief Concatenate binary data to SDS string
 * @param s SDS string
 * @param t Data to append
 * @param len Length of data to append
 * @return Reallocated SDS string, or nullptr on failure
 */
sds sdscat( sds s, const void* t, size_t len ) {
  if ( s == nullptr || t == nullptr ) return s;

  s = sdsMakeRoomFor( s, len );
  if ( s == nullptr ) return nullptr;

  size_t curlen = sdslen( s );
  memcpy( s + curlen, t, len );
  sdssetlen( s, curlen + len );
  s[ curlen + len ] = '\0';

  return s;
}

/**
 * @brief Concatenate C string to SDS string
 * @param s SDS string
 * @param t C string to append
 * @return Reallocated SDS string, or nullptr on failure
 */
sds sdscat( sds s, const char* t ) {
  if ( t == nullptr ) return s;
  return sdscat( s, t, strlen( t ) );
}

/**
 * @brief Concatenate another SDS string to SDS string
 * @param s SDS string
 * @param t SDS string to append
 * @return Reallocated SDS string, or nullptr on failure
 */
sds sdscat( sds s, const sds t ) {
  if ( t == nullptr ) return s;
  return sdscat( s, static_cast< const void* >( t ), sdslen( t ) );
}

/**
 * @brief Copy binary data to SDS string
 * @param s SDS string
 * @param t Data to copy
 * @param len Length of data to copy
 * @return Reallocated SDS string, or nullptr on failure
 */
sds sdscpy( sds s, const void* t, size_t len ) {
  if ( s == nullptr ) return nullptr;
  if ( t == nullptr ) len = 0;

  if ( sdsalloc( s ) < len ) {
    s = sdsMakeRoomFor( s, len - sdslen( s ) );
    if ( s == nullptr ) return nullptr;
  }

  if ( len > 0 && t != nullptr ) {
    memcpy( s, t, len );
  }
  s[ len ] = '\0';
  sdssetlen( s, len );

  return s;
}

/**
 * @brief Copy C string to SDS string
 * @param s SDS string
 * @param t C string to copy
 * @return Reallocated SDS string, or nullptr on failure
 */
sds sdscpy( sds s, const char* t ) {
  if ( t == nullptr ) return sdscpy( s, "", 0 );
  return sdscpy( s, t, strlen( t ) );
}

/**
 * @brief Copy another SDS string to SDS string
 * @param s SDS string
 * @param t SDS string to copy
 * @return Reallocated SDS string, or nullptr on failure
 */
sds sdscpy( sds s, const sds t ) {
  if ( t == nullptr ) return sdscpy( s, "", 0 );
  return sdscpy( s, t, sdslen( t ) );
}

/**
 * @brief Duplicate an SDS string
 * @param s SDS string to duplicate
 * @return New SDS string copy, or nullptr on failure
 */
sds sdsdup( const sds s ) {
  if ( s == nullptr ) return nullptr;
  auto result = sdsnewlen( s, sdslen( s ) );
  return result ? *result : nullptr;
}

/**
 * @brief Compare two SDS strings
 * @param s1 First SDS string
 * @param s2 Second SDS string
 * @return < 0 if s1 < s2, 0 if s1 == s2, > 0 if s1 > s2
 */
int8_t sdscmp( const sds s1, const sds s2 ) {
  if ( s1 == s2 ) return 0;
  if ( s1 == nullptr ) return -1;
  if ( s2 == nullptr ) return 1;

  size_t l1     = sdslen( s1 );
  size_t l2     = sdslen( s2 );
  size_t minlen = ( l1 < l2 ) ? l1 : l2;

  int cmp = memcmp( s1, s2, minlen );
  if ( cmp == 0 ) {
    if ( l1 < l2 ) return -1;
    if ( l1 > l2 ) return 1;
    return 0;
  }
  return cmp;
}
