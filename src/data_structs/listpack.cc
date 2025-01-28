#include "xmalloc.h"
#include <config.h>
#include <cstddef>
#include <cstdint>
#include <listpack.h>
#include <optional>

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-23
 * @brief 1 + 7 编码
 *
 * 0               1                7
 * |  encode_type  |   uint data    |
 */
constexpr const size_t lp_encoding_7bit_uint            = 0;
constexpr const size_t lp_encoding_7bit_uint_mask       = 0x80;
constexpr const size_t lp_encoding_7bit_uint_entry_size = 2;

#define LP_ENCODING_INT_MAGIC( bit_size, flag, mask, entry_size )                \
  constexpr const size_t lp_encoding_##bit_size##bit_int            = flag;      \
  constexpr const size_t lp_encoding_##bit_size##bit_int_mask       = mask;      \
  constexpr const size_t lp_encoding_##bit_size##bit_int_entry_size = entry_size

#define LP_ENCODING_UINT_MAGIC( bit_size, flag, mask, entry_size )                \
  constexpr const size_t lp_encoding_##bit_size##bit_uint            = flag;      \
  constexpr const size_t lp_encoding_##bit_size##bit_uint_mask       = mask;      \
  constexpr const size_t lp_encoding_##bit_size##bit_uint_entry_size = entry_size

#define LP_ENCODING_STR_MAGIC( bit_size, flag, mask, entry_size )                \
  constexpr const size_t lp_encoding_##bit_size##bit_str            = flag;      \
  constexpr const size_t lp_encoding_##bit_size##bit_str_mask       = mask;      \
  constexpr const size_t lp_encoding_##bit_size##bit_str_entry_size = entry_size

#define LP_ENCODING_MAGIC( type, bit, flag, mask, entry_size ) \
  LP_ENCODING_##type##_MAGIC( bit, flag, mask, entry_size )

LP_ENCODING_MAGIC( INT, 7, 0, 0x80, 2 );

constexpr static inline void lpSetTotalBytes( listpack lp, uint32_t size ) {
  //! QUESTION: can use for loop?
  lp[ 0 ] = ( size >> 0 ) & 0xFF;
  lp[ 1 ] = ( size >> 8 ) & 0xFF;
  lp[ 2 ] = ( size >> 16 ) & 0xFF;
  lp[ 3 ] = ( size >> 24 ) & 0xFF;
}

constexpr static inline void lpSetNumElements( listpack lp, uint32_t size ) {
  lp[ 4 ] = ( size >> 0 ) & 0xFF;
  lp[ 5 ] = ( size >> 8 ) & 0xFF;
}

constexpr static inline size_t lpGetTotalBytes( listpack lp ) {
  return lp[ 0 ] << 0 | lp[ 1 ] << 8 | lp[ 2 ] << 16 | lp[ 3 ] << 24;
}

constexpr static inline size_t lpGetNumElements( listpack lp ) {
  return lp[ 5 ] << 0 | lp[ 6 ] << 8;
}

constexpr static inline bool isIntegrity( listpack lp, listpack p ) {
  return p >= lp + lp_hdr_size && p < lp + lpGetTotalBytes( lp );
}

constexpr static listpack lpSkip( listpack lp ) {}

constexpr static std::optional< listpack > lpInsert( listpack    lp,
                                                     const char* ele_str,
                                                     const char* ele_int,
                                                     uint32_t    size,
                                                     listpack    p,
                                                     insert_type where,
                                                     char**      new_p ) {
  if ( ele_str == nullptr && ele_int == nullptr ) {
    where = insert_type::lp_replace;
  }

  if ( where == insert_type::lp_after ) {
    p     = lpSkip( p );
    where = insert_type::lp_before;
    rediscc_assert( isIntegrity( lp, p ), "" );
  }
}

std::optional< listpack > lpNew( size_t capacity ) {
  uint32_t size = capacity > ( lp_hdr_size + lp_eof_size ) ? capacity : lp_hdr_size + lp_eof_size;
  listpack lp   = (listpack)xmalloc_usable( size, nullptr );
  if ( lp == nullptr ) {
    return std::nullopt;
  }
  lpSetTotalBytes( lp, size );
  lpSetNumElements( lp, 0 );
  lp[ lp_hdr_size ] = lp_eof;

  return lp;
}

constexpr void lpFree( listpack lp ) { xfree( lp ); }

std::optional< listpack > lpShrinkToFit( listpack lp ) {
  size_t size = lpGetTotalBytes( lp );
  if ( size < xmalloc_size( lp ) ) {
    return (listpack)xrealloc_usable( lp, size, nullptr );
  } else {
    return lp;
  }
}
