/* quicklist.h - A generic doubly linked quicklist implementation
 *
 * Copyright (c) 2014, Matt Stancliff <matt@genges.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this quicklist of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this quicklist of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef REDISCC_DATA_STRUCTURE_QUICK_LIST_H
#define REDISCC_DATA_STRUCTURE_QUICK_LIST_H

#include <cstddef>
#include <cstdint>

typedef struct quicklistNode {
  struct quicklistNode* prev;
  struct quicklistNode* next;
  unsigned char*        entry;
  size_t                sz;
  unsigned int          count              : 16;
  unsigned int          encoding           : 2;
  unsigned int          container          : 2;
  unsigned int          recompress         : 1;
  unsigned int          attempted_compress : 1;
  unsigned int          dont_compress      : 1;
  unsigned int          extra              : 9;
} quicklistNode;

typedef struct quicklistLZF {
  size_t sz; /* LZF size in bytes*/
  char   compressed[];
} quicklistLZF;

typedef struct quicklistBookmark {
  quicklistNode* node;
  char*          name;
} quicklistBookmark;

#if UINTPTR_MAX == 0xffff'ffff
constexpr const size_t QL_FILL_BITS = 14;
constexpr const size_t QL_COMP_BITS = 14;
constexpr const size_t QL_BM_BITS   = 4;
#elif UINTPTR_MAX == 0xffff'ffff'ffff'ffff
constexpr const size_t QL_FILL_BITS = 16;
constexpr const size_t QL_COMP_BITS = 16;
constexpr const size_t QL_BM_BITS   = 4;
#else
#  error "unknown arch bits count
#endif

typedef struct quicklist {
  quicklistNode*    head;
  quicklistNode*    tail;
  unsigned long     count;
  unsigned long     len;
  signed int        fill : QL_FILL_BITS;
  unsigned int      compress : QL_COMP_BITS;
  unsigned int      bookmark_count : QL_BM_BITS;
  quicklistBookmark bookmarks[];
} quicklist;

typedef struct quicklistIter {
  quicklist*     quicklist;
  quicklistNode* current;
  unsigned char* zi;     /* points to the current element */
  long           offset; /* offset in current listpack */
  int            direction;
} quicklistIter;

typedef struct quicklistEntry {
  const quicklist* quicklist;
  quicklistNode*   node;
  unsigned char*   zi;
  unsigned char*   value;
  long long        longval;
  size_t           sz;
  int              offset;
} quicklistEntry;

#endif  //! REDISCC_DATA_STRUCTURE_QUICK_LIST_H
