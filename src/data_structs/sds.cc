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
  sdssetlen( buf, initlen );
  sdssetalloc( buf, usable );
  uint8_t* flag = ( (uint8_t*)buf ) - 1;
  *flag         = static_cast< uint8_t >( hdr_type );

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
