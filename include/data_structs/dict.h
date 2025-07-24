/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2) or the Server Side Public License v1 (SSPLv1).
 */

#ifndef REDISCC_DATA_STRUCTURE_DICT_H
#define REDISCC_DATA_STRUCTURE_DICT_H

#include <cstddef>
#include <cstdint>

// Forward declarations
typedef struct dictEntry dictEntry;
typedef struct dict      dict;

// Dictionary entry structure
struct dictEntry {
  void* key;
  union {
    void*    val;
    uint64_t u64;
    int64_t  s64;
    double   d;
  } v;
  dictEntry* next;
};

typedef struct dictType {
  uint64_t ( *hashFunction )( const void* key );
  void* ( *keyDup )( dict* d, const void* key );
  void* ( *valDup )( dict* d, const void* obj );
  int ( *keyCompare )( dict* d, const void* key1, const void* key2 );
  void ( *keyDestructor )( dict* d, void* key );
  void ( *valDestructor )( dict* d, void* obj );
  int ( *resizeAllowed )( size_t moreMem, double usedRatio );
  void ( *rehashingStarted )( dict* d );
  void ( *rehashingCompleted )( dict* d );
  size_t ( *dictMetadataBytes )( dict* d );
  void*        userdata;
  unsigned int no_value     : 1;
  unsigned int keys_are_odd : 1;
  int ( *storedKeyCompare )( dict* d, const void* key1, const void* key2 );
  void ( *onDictRelease )( dict* d );
} dictType;

#define DICTHT_SIZE( exp )      ( ( exp ) == -1 ? 0 : (unsigned long)1 << ( exp ) )
#define DICTHT_SIZE_MASK( exp ) ( ( exp ) == -1 ? 0 : ( DICTHT_SIZE( exp ) ) - 1 )

struct dict {
  dictType*     type;
  dictEntry**   ht_table[ 2 ];
  unsigned long ht_used[ 2 ];
  long          rehashidx;
  unsigned      pauserehash     : 15;
  unsigned      useStoredKeyApi : 1;
  signed char   ht_size_exp[ 2 ];
  int16_t       pauseAutoResize;
  void*         metadata[];
};

typedef struct dictIterator {
  dict*              d;
  long               index;
  int                table, safe;
  dictEntry *        entry, *nextEntry;
  unsigned long long fingerprint;
} dictIterator;

typedef struct dictStats {
  int            htidx;
  unsigned long  buckets;
  unsigned long  maxChainLen;
  unsigned long  totalChainLen;
  unsigned long  htSize;
  unsigned long  htUsed;
  unsigned long* clvector;
} dictStats;

typedef void( dictScanFunction )( void* privdata, const dictEntry* de );
typedef void*(dictDefragAllocFunction)( void* ptr );
typedef struct {
  dictDefragAllocFunction* defragAlloc; /* Used for entries etc. */
  dictDefragAllocFunction* defragKey;   /* Defrag-realloc keys (optional) */
  dictDefragAllocFunction* defragVal;   /* Defrag-realloc values (optional) */
} dictDefragFunctions;

// Dictionary status codes
#define DICT_OK  0
#define DICT_ERR 1

// API Functions
dict*      dictCreate( dictType* type );
void       dictRelease( dict* d );
int        dictAdd( dict* d, void* key, void* val );
int        dictReplace( dict* d, void* key, void* val );
int        dictDelete( dict* d, const void* key );
dictEntry* dictFind( dict* d, const void* key );
void*      dictFetchValue( dict* d, const void* key );
size_t     dictSize( dict* d );
int        dictIsEmpty( dict* d );
int        dictRehash( dict* d, int n );
int        dictResize( dict* d );
int        dictExpand( dict* d, unsigned long size );
int        dictIsRehashing( dict* d );
dictEntry* dictAddRaw( dict* d, void* key, dictEntry** existing );

// Macros for accessing dictionary entries
#define dictFreeVal( d, entry )                                                                    \
  if ( ( d )->type->valDestructor ) ( d )->type->valDestructor( ( d ), ( entry )->v.val )

#define dictSetVal( d, entry, _val_ )                                                              \
  do {                                                                                             \
    if ( ( d )->type->valDup )                                                                     \
      ( entry )->v.val = ( d )->type->valDup( ( d ), _val_ );                                      \
    else                                                                                           \
      ( entry )->v.val = ( _val_ );                                                                \
  } while ( 0 )

#define dictSetSignedIntegerVal( entry, _val_ )                                                    \
  do {                                                                                             \
    ( entry )->v.s64 = _val_;                                                                      \
  } while ( 0 )

#define dictSetUnsignedIntegerVal( entry, _val_ )                                                  \
  do {                                                                                             \
    ( entry )->v.u64 = _val_;                                                                      \
  } while ( 0 )

#define dictSetDoubleVal( entry, _val_ )                                                           \
  do {                                                                                             \
    ( entry )->v.d = _val_;                                                                        \
  } while ( 0 )

#define dictFreeKey( d, entry )                                                                    \
  if ( ( d )->type->keyDestructor ) ( d )->type->keyDestructor( ( d ), ( entry )->key )

#define dictSetKey( d, entry, _key_ )                                                              \
  do {                                                                                             \
    if ( ( d )->type->keyDup )                                                                     \
      ( entry )->key = ( d )->type->keyDup( ( d ), _key_ );                                        \
    else                                                                                           \
      ( entry )->key = ( _key_ );                                                                  \
  } while ( 0 )

#define dictCompareKeys( d, key1, key2 )                                                           \
  ( ( ( d )->type->keyCompare ) ? ( d )->type->keyCompare( ( d ), key1, key2 )                     \
                                : ( key1 ) == ( key2 ) )

#define dictHashKey( d, key )           ( d )->type->hashFunction( key )
#define dictGetKey( he )                ( ( he )->key )
#define dictGetVal( he )                ( ( he )->v.val )
#define dictGetSignedIntegerVal( he )   ( ( he )->v.s64 )
#define dictGetUnsignedIntegerVal( he ) ( ( he )->v.u64 )
#define dictGetDoubleVal( he )          ( ( he )->v.d )

// Hash table size macros (remove duplicate definitions)

// Global resize control functions
void dictEnableResize( void );
void dictDisableResize( void );

// Hash functions
uint64_t dictGenHashFunction( const void* key, size_t len );

// Utility functions
void dictEmpty( dict* d );

#endif  //! REDISCC_DATA_STRUCTURE_DICT_H
