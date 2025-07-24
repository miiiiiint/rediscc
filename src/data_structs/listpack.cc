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
[[maybe_unused]]
constexpr const size_t lp_encoding_7bit_uint
    = 0;
[[maybe_unused]]
constexpr const size_t lp_encoding_7bit_uint_mask
    = 0x80;
[[maybe_unused]]
constexpr const size_t lp_encoding_7bit_uint_entry_size
    = 2;

#define LP_ENCODING_INT_MAGIC( bit_size, flag, mask, entry_size )                                  \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_int                                           \
      = flag;                                                                                      \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_int_mask                                      \
      = mask;                                                                                      \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_int_entry_size                                \
      = entry_size

#define LP_ENCODING_UINT_MAGIC( bit_size, flag, mask, entry_size )                                 \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_uint                                          \
      = flag;                                                                                      \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_uint_mask                                     \
      = mask;                                                                                      \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_uint_entry_size                               \
      = entry_size

#define LP_ENCODING_STR_MAGIC( bit_size, flag, mask, entry_size )                                  \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_str                                           \
      = flag;                                                                                      \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_str_mask                                      \
      = mask;                                                                                      \
  [[maybe_unused]]                                                                                 \
  constexpr const size_t lp_encoding_##bit_size##bit_str_entry_size                                \
      = entry_size

#define LP_ENCODING_MAGIC( type, bit, flag, mask, entry_size )                                     \
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

[[maybe_unused]]
constexpr static inline size_t lpGetNumElements( listpack lp ) {
  return lp[ 5 ] << 0 | lp[ 6 ] << 8;
}

constexpr static inline bool isIntegrity( listpack lp, listpack p ) {
  return p >= lp + lp_hdr_size && p < lp + lpGetTotalBytes( lp );
}

/**
 * @brief Skip current element and return pointer to next element
 * @param p Pointer to current element
 * @return Pointer to next element, or nullptr if at end
 */
constexpr static listpack lpSkip( listpack p ) {
  if ( p == nullptr ) return nullptr;

  uint8_t first_byte = *p;

  // Check encoding type and calculate element size
  if ( ( first_byte & 0x80 ) == 0 ) {
    // 7-bit unsigned integer: 0xxxxxxx
    return p + 1;
  } else if ( ( first_byte & 0xC0 ) == 0x80 ) {
    // 6-bit string length: 10xxxxxx + string data
    size_t len = first_byte & 0x3F;
    return p + 1 + len;
  } else if ( ( first_byte & 0xE0 ) == 0xC0 ) {
    // 13-bit string length: 110xxxxx + next byte + string data
    if ( p[ 1 ] == 0 ) return nullptr;  // Invalid
    size_t len = ( ( first_byte & 0x1F ) << 8 ) | p[ 1 ];
    return p + 2 + len;
  } else if ( ( first_byte & 0xF0 ) == 0xE0 ) {
    // 32-bit string length: 1110xxxx + 4 bytes length + string data
    uint32_t len = ( p[ 1 ] << 0 ) | ( p[ 2 ] << 8 ) | ( p[ 3 ] << 16 ) | ( p[ 4 ] << 24 );
    return p + 5 + len;
  } else if ( first_byte == 0xF1 ) {
    // 24-bit signed integer
    return p + 4;
  } else if ( first_byte == 0xF2 ) {
    // 32-bit signed integer
    return p + 5;
  } else if ( first_byte == 0xF3 ) {
    // 64-bit signed integer
    return p + 9;
  } else if ( first_byte == 0xF4 ) {
    // 24-bit unsigned integer
    return p + 4;
  } else if ( first_byte == 0xF5 ) {
    // 32-bit unsigned integer
    return p + 5;
  } else if ( first_byte == 0xF6 ) {
    // 64-bit unsigned integer
    return p + 9;
  } else if ( first_byte == 0xFF ) {
    // End of listpack
    return nullptr;
  } else {
    // Invalid encoding
    return nullptr;
  }
}

/**
 * @brief Insert element into listpack
 * @param lp Listpack pointer
 * @param ele_str String element to insert (if not null)
 * @param ele_int Integer element to insert (if not null)
 * @param size Size of element
 * @param p Position to insert at
 * @param where Insert position (before/after/replace)
 * @param new_p Output pointer to new element position
 * @return New listpack pointer, or nullopt on failure
 */
[[maybe_unused]]
constexpr static std::optional< listpack > lpInsert( listpack    lp,
                                                     const char* ele_str,
                                                     const char* ele_int,
                                                     uint32_t    size,
                                                     listpack    p,
                                                     insert_type where,
                                                     char**      new_p ) {
  if ( lp == nullptr ) return std::nullopt;

  // For now, implement a basic version that just returns the original listpack
  // This is a placeholder implementation
  (void)ele_str;
  (void)ele_int;
  (void)size;
  (void)new_p;

  if ( ele_str == nullptr && ele_int == nullptr ) {
    where = insert_type::lp_replace;
  }

  if ( where == insert_type::lp_after ) {
    p     = lpSkip( p );
    where = insert_type::lp_before;
    if ( !isIntegrity( lp, p ) ) {
      return std::nullopt;
    }
  }

  // Basic implementation: just return the original listpack
  // In a full implementation, this would:
  // 1. Calculate the size needed for the new element
  // 2. Reallocate the listpack if necessary
  // 3. Move existing elements to make space
  // 4. Encode and insert the new element
  // 5. Update the listpack header

  return lp;
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
