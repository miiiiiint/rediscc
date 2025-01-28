/* adlist.h - A generic doubly linked adlist implementation
 *
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2) or the Server Side Public License v1 (SSPLv1).
 */

#ifndef REDISCC_DATA_STRUCTURE_ADLIST_H
#define REDISCC_DATA_STRUCTURE_ADLIST_H

#include <cstddef>
#include <cstdint>
#include <functional>

typedef struct listNode {
  struct listNode* prev;
  struct listNode* next;
  void*            value;
} listNode;

typedef struct listIter {
  listNode* next;
  int       direction;
} listIter;

using func_dup   = std::function< void*( void* ) >;
using func_free  = std::function< void*( void* ) >;
using func_match = std::function< int32_t( void*, void* ) >;

typedef struct adlist {
  listNode*     head;
  listNode*     tail;
  func_dup      dup;
  func_free     free;
  func_match    match;
  unsigned long len;
} adlist;

constexpr size_t    list_length( const adlist* l ) { return l->len; }
constexpr listNode* list_first( const adlist* l ) { return l->head; }
constexpr listNode* list_last( const adlist* l ) { return l->tail; }
constexpr listNode* list_prev_node( const listNode* n ) { return n->prev; }
constexpr listNode* list_next_node( const listNode* n ) { return n->next; }
constexpr void*     list_node_value( const listNode* n ) { return n->value; }
constexpr void      list_set_dup_method( adlist* l, func_dup&& m ) { l->dup = m; }
constexpr void      list_set_free_method( adlist* l, func_free&& m ) { l->free = m; }
constexpr void      list_set_match_method( adlist* l, func_match&& m ) { l->match = m; }
inline func_dup     list_get_dup_method( const adlist* l ) { return l->dup; }
inline func_free    list_get_free_method( const adlist* l ) { return l->free; }
inline func_match   list_get_match_method( const adlist* l ) { return l->match; }

/* Prototypes */
adlist*   listCreate( void );
void      listRelease( adlist* adlist );
void      listEmpty( adlist* adlist );
adlist*   listAddNodeHead( adlist* adlist, void* value );
adlist*   listAddNodeTail( adlist* adlist, void* value );
adlist*   listInsertNode( adlist* adlist, listNode* old_node, void* value, int after );
void      listDelNode( adlist* adlist, listNode* node );
listIter* listGetIterator( adlist* adlist, int direction );
listNode* listNext( listIter* iter );
void      listReleaseIterator( listIter* iter );
adlist*   listDup( adlist* orig );
listNode* listSearchKey( adlist* adlist, void* key );
listNode* listIndex( adlist* adlist, long index );
void      listRewind( adlist* adlist, listIter* li );
void      listRewindTail( adlist* adlist, listIter* li );
void      listRotateTailToHead( adlist* adlist );
void      listRotateHeadToTail( adlist* adlist );
void      listJoin( adlist* l, adlist* o );
void      listInitNode( listNode* node, void* value );
void      listLinkNodeHead( adlist* adlist, listNode* node );
void      listLinkNodeTail( adlist* adlist, listNode* node );
void      listUnlinkNode( adlist* adlist, listNode* node );

#endif  //! REDISCC_DATA_STRUCTURE_ADLIST_H
