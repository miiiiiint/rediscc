/*
 * Copyright (c) 2009-2012, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2009-current, Redis Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
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

#ifndef REDISCC_DATA_STRUCTURE_INT_SET_H
#define REDISCC_DATA_STRUCTURE_INT_SET_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

enum class encoding_type : uint8_t {
  ENC_INT16,
  ENC_INT32,
  ENC_INT64,
};

typedef struct intset {
  encoding_type encoding;
  uint32_t      length;
  int8_t        contents[];
} intset;

std::optional< intset* >                    intsetNew( void );
std::pair< bool, std::optional< intset* > > intsetAdd( intset* is, int64_t value );
std::pair< bool, std::optional< intset* > > intsetRemove( intset* is, int64_t value );
bool                                        intsetFind( intset* is, int64_t value );
int64_t                                     intsetRandom( intset* is );
int64_t                                     intsetMax( intset* is );
int64_t                                     intsetMin( intset* is );
std::pair< bool, std::optional< int64_t > > intsetGet( intset* is, uint32_t pos );
constexpr uint32_t                          intsetLen( const intset* is );
constexpr size_t                            intsetBlobLen( intset* is );
bool intsetValidateIntegrity( const unsigned char* p, size_t size, int deep );

#endif  //! REDISCC_DATA_STRUCTURE_INT_SET_H
