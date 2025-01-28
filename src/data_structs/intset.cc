#include "config.h"
#include "endianconv.h"
#include "xmalloc.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <intset.h>
#include <limits>
#include <optional>
#include <random>
#include <utility>

constexpr static inline encoding_type _intsetValueEncoding( int64_t v ) {
  if ( v < std::numeric_limits< int32_t >::min() || v > std::numeric_limits< int32_t >::max() ) {
    return encoding_type::ENC_INT64;
  } else if ( v < std::numeric_limits< int16_t >::min()
              || v > std::numeric_limits< int16_t >::max() ) {
    return encoding_type::ENC_INT32;
  }
  return encoding_type::ENC_INT16;
}

constexpr static int64_t _intsetGetEncoding( intset* is, int32_t pos, encoding_type type ) {
  void* val;
  switch ( type ) {
  case encoding_type::ENC_INT16:
    memcpy( &val, ( (int16_t*)is->contents ) + pos, sizeof( int16_t ) );
    memrev_ifbe< int16_t >( val );
    return *reinterpret_cast< int16_t* >( val );
  case encoding_type::ENC_INT32:
    memcpy( &val, ( (int32_t*)is->contents ) + pos, sizeof( int32_t ) );
    memrev_ifbe< int32_t >( val );
    return *reinterpret_cast< int32_t* >( val );
  case encoding_type::ENC_INT64:
    memcpy( &val, ( (int64_t*)is->contents ) + pos, sizeof( int64_t ) );
    memrev_ifbe< int64_t >( val );
    return *reinterpret_cast< int64_t* >( val );
  }
}

constexpr static int64_t _intsetGet( intset* is, int32_t pos ) {
  return _intsetGetEncoding( is, pos, is->encoding );
}

constexpr static void _intsetSet( intset* is, int32_t pos, int64_t value ) {
  switch ( is->encoding ) {
  case encoding_type::ENC_INT64:
    reinterpret_cast< int64_t* >( is->contents )[ pos ] = value;
    memrev_ifbe< int64_t >( is->contents + pos );
    break;
  case encoding_type::ENC_INT32:
    reinterpret_cast< int32_t* >( is->contents )[ pos ] = value;
    memrev_ifbe< int32_t >( is->contents + pos );
    break;
  case encoding_type::ENC_INT16:
    reinterpret_cast< int16_t* >( is->contents )[ pos ] = value;
    memrev_ifbe< int16_t >( is->contents + pos );
    break;
  }
}

constexpr static inline size_t _intsetTypeSize( encoding_type type ) {
  switch ( type ) {
  case encoding_type::ENC_INT16: return sizeof( int16_t );
  case encoding_type::ENC_INT32: return sizeof( int32_t );
  case encoding_type::ENC_INT64: return sizeof( int64_t );
  }
}

constexpr static std::optional< intset* > intsetResize( intset* is, uint32_t len ) {
  uint64_t size = (uint64_t)len * _intsetTypeSize( is->encoding );
  rediscc_assert( size <= SIZE_MAX - sizeof( intset ),
                  "resize size overflow! size({}) vs max({})",
                  size,
                  SIZE_MAX - sizeof( intset ) );
  is = (intset*)xrealloc( is, sizeof( intset ) + size );
  if ( is == nullptr ) return std::nullopt;
  return is;
}

constexpr static std::optional< intset* > intsetUpgradeAndAdd( intset* is, int64_t value ) {
  encoding_type cur_type = is->encoding;
  encoding_type new_type = _intsetValueEncoding( value );
  int32_t       length   = intrev_ifbe( is->length );
  int32_t       prepend  = value < 0 ? 1 : 0;

  is->encoding = new_type;
  is           = intsetResize( is, intrev_ifbe( is->length ) + 1 ).value_or( nullptr );
  if ( is == nullptr ) {
    return std::nullopt;
  }

  while ( length-- ) {
    _intsetSet( is, length + prepend, _intsetGetEncoding( is, length, cur_type ) );
  }
  if ( prepend ) {
    _intsetSet( is, 0, value );
  } else {
    _intsetSet( is, intrev_ifbe( is->length ), value );
  }

  is->length = intrev_ifbe( intrev_ifbe( is->length ) + 1 );
  return is;
}

constexpr static void intsetMoveTail( intset* is, uint32_t from, uint32_t to ) {
  void *   src, *dst;
  uint32_t bytes = intrev_ifbe( is->length ) - from;

  switch ( is->encoding ) {
  case encoding_type::ENC_INT64:
    src = reinterpret_cast< int64_t* >( is->contents ) + from;
    dst = reinterpret_cast< int64_t* >( is->contents ) + to;
    break;
  case encoding_type::ENC_INT32:
    src = reinterpret_cast< int32_t* >( is->contents ) + from;
    dst = reinterpret_cast< int32_t* >( is->contents ) + to;
    break;
  case encoding_type::ENC_INT16:
    src = reinterpret_cast< int16_t* >( is->contents ) + from;
    dst = reinterpret_cast< int16_t* >( is->contents ) + to;
    break;
  }

  bytes *= _intsetTypeSize( is->encoding );
  memmove( dst, src, bytes );
}

constexpr static std::pair< bool, uint32_t > intsetSearch( intset* is, int64_t value ) {
  int32_t pos_min = 0, pos_max = intrev_ifbe( is->length ) - 1, pos_mid = -1;
  int64_t cur = -1;

  if ( intrev_ifbe( is->length ) == 0 ) {
    return { false, 0 };
  } else {
    if ( value > _intsetGet( is, pos_max ) ) {
      return { false, intrev_ifbe( is->length ) };
    } else if ( value < _intsetGet( is, 0 ) ) {
      return { false, 0 };
    }
  }

  while ( pos_max >= pos_min ) {
    pos_mid = ( (uint32_t)pos_min + (uint32_t)pos_max ) >> 1;
    cur     = _intsetGet( is, pos_mid );
    if ( value > cur )
      pos_min = pos_mid + 1;
    else if ( value < cur )
      pos_max = pos_mid - 1;
    else
      break;
  }

  if ( value == cur ) {
    return { true, pos_mid };
  }
  return { false, pos_min };
}

std::optional< intset* > intsetNew( void ) {
  intset* is = (intset*)xmalloc( sizeof( intset ) );
  if ( is == nullptr ) {
    REDISCC_WARN( "cannot malloc a new intset" );
    return std::nullopt;
  }

  is->encoding = encoding_type::ENC_INT16;
  is->length   = 0;

  return is;
}

std::pair< bool, std::optional< intset* > > intsetAdd( intset* is, int64_t value ) {
  encoding_type type = _intsetValueEncoding( value );

  if ( type > encoding_type::ENC_INT32 ) {
    return { true, intsetUpgradeAndAdd( is, value ) };
  } else {
    auto [ ok, pos ] = intsetSearch( is, value );
    if ( ok ) {
      return { false, is };
    }
    is = intsetResize( is, intrev_ifbe( is->length ) + 1 ).value_or( nullptr );
    if ( is == nullptr ) {
      return { false, std::nullopt };
    }

    if ( pos < intrev_ifbe( is->length ) ) {
      intsetMoveTail( is, pos, pos + 1 );
    }

    _intsetSet( is, pos, value );
    is->length = intrev_ifbe( intrev_ifbe( is->length ) + 1 );
    return { true, is };
  }
}

std::pair< bool, std::optional< intset* > > intsetRemove( intset* is, int64_t value ) {
  encoding_type type = _intsetValueEncoding( value );

  if ( auto [ ok, pos ] = intsetSearch( is, value ); type <= is->encoding && ok ) {
    uint32_t len = intrev_ifbe( is->length );
    if ( pos < len - 1 ) intsetMoveTail( is, pos + 1, pos );
    is = intsetResize( is, len - 1 ).value_or( nullptr );
    if ( is == nullptr ) {
      return { false, std::nullopt };
    }
    is->length = intrev_ifbe( is->length - 1 );

    return { true, is };
  }
  return { false, is };
}

bool intsetFind( intset* is, int64_t value ) {
  encoding_type type = _intsetValueEncoding( value );
  auto [ ok, _ ]     = intsetSearch( is, value );
  return type <= is->encoding && ok;
}

int64_t intsetRandom( intset* is ) {
  uint32_t                                  len = intrev_ifbe( is->length );
  static std::random_device                 rd;
  static std::mt19937                       gen( rd() );
  std::uniform_int_distribution< uint32_t > dist( 0, len - 1 );

  return _intsetGet( is, dist( gen ) );
}

int64_t intsetMax( intset* is ) {
  uint32_t len = intrev_ifbe( is->length );
  return _intsetGet( is, len - 1 );
}

int64_t intsetMin( intset* is ) { return _intsetGet( is, 0 ); }

std::pair< bool, std::optional< int64_t > > intsetGet( intset* is, uint32_t pos ) {
  if ( pos < intrev_ifbe( is->length ) ) {
    return { true, _intsetGet( is, pos ) };
  }
  return { false, std::nullopt };
}

constexpr uint32_t intsetLen( const intset* is ) { return intrev_ifbe( is->length ); }

constexpr size_t intsetBlobLen( intset* is ) {
  return sizeof( intset ) + (size_t)intrev_ifbe( is->length ) * _intsetTypeSize( is->encoding );
}

bool intsetValidateIntegrity( const unsigned char* p, size_t size, int deep ) {
  (void)p;
  (void)size;
  (void)deep;
  return true;
}
