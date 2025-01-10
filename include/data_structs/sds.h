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

#include <cstddef>
#include <cstdint>
#include <memory>

template < typename T >
concept IsUnsignedInteger =
  std::is_same_v< T, uint8_t > || std::is_same_v< T, uint16_t > ||
  std::is_same_v< T, uint32_t > || std::is_same_v< T, uint64_t >;


/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-10
 * @brief 
 * 
 * @tparam SDSHDR_TYPE 
 */
template < IsUnsignedInteger SDSHDR_TYPE >
class [[gnu::packed]] sds_t {
private:
  SDSHDR_TYPE                  len_;   ///<
  SDSHDR_TYPE                  alloc_; ///<
  uint8_t                      flags_; ///<
  std::unique_ptr< uint8_t[] > buf_;   ///<

public:
public:
  size_t length() const;
};

#endif //! __REDISCC_DATA_STRUCTURES_SDS_H
