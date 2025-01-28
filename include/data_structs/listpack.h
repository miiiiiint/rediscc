/* Listpack -- A lists of strings serialization format
 *
 * This file implements the specification you can find at:
 *
 *  https://github.com/antirez/listpack
 *
 * Copyright (c) 2017-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2) or the Server Side Public License v1 (SSPLv1).
 */

#ifndef REDISCC_DATA_STRUCTURE_LIST_PACK_H
#define REDISCC_DATA_STRUCTURE_LIST_PACK_H

#include <cstddef>
#include <cstdint>
#include <optional>

using listpack = uint8_t*;

constexpr const size_t lp_hdr_size = 6;  // 4 bytes total size
                                         // 2 bytes element size
constexpr const size_t lp_eof_size = 1;  // 1bytes eof
constexpr const size_t lp_eof      = 0xFF;

typedef struct {
  uint8_t* sval;
  uint32_t slen;
  uint64_t lval;
} listpackEntry;

enum class insert_type : uint8_t {
  lp_before  = 0,
  lp_after   = 1,
  lp_replace = 2,
};

std::optional< listpack > lpNew( size_t capacity );
constexpr void            lpFree( listpack lp );
std::optional< listpack > lpShrinkToFit( listpack lp );

#endif  //! REDISCC_DATA_STRUCTURE_LIST_PACK_H
