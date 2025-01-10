#ifndef REDISCC_ATOMICVAR_H
#define REDISCC_ATOMICVAR_H

#include <concepts>
#include <type_traits>
#ifdef __has_include
#if __has_include( <atomic> )
#include <atomic>
#endif
#else
#error "must support <atomic>"
#endif

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-10
 * @brief 
 * 
 * @tparam T 
 */
template < typename T >
concept IsAtomic = requires( T t, typename T::value_type v ) {
  std::is_same_v< T, std::atomic< typename T::value_type > >;
  { t.load() } -> std::same_as< typename T::value_type >;
  { t.store( v ) };
};

template < IsAtomic Atomic >
void atomic_inc( Atomic& var, typename Atomic::value_type count ) {
  atomic_fetch_add_explicit( &var, count, std::memory_order_relaxed );
}

template < IsAtomic Atomic >
void atomic_get_inc( Atomic& var, typename Atomic::value_type& oldvalue_var,
                     typename Atomic::value_type count ) {
  oldvalue_var =
    atomic_fetch_add_explicit( &var, count, std::memory_order_relaxed );
}

template < IsAtomic Atomic >
void atomic_inc_get( Atomic& var, typename Atomic::value_type& newvalue_var,
                     typename Atomic::value_type count ) {
  newvalue_var =
    atomic_fetch_add_explicit( &var, count, std::memory_order_relaxed ) + count;
}

template < IsAtomic Atomic >
void atomic_dec( Atomic& var, typename Atomic::value_type count ) {
  atomic_fetch_sub_explicit( &var, count, std::memory_order_relaxed );
}

template < IsAtomic Atomic >
void atomic_get( Atomic& var, typename Atomic::value_type& dstvar ) {
  dstvar = atomic_load_explicit( &var, std::memory_order_relaxed );
}

template < IsAtomic Atomic >
void atomic_set( Atomic& var, typename Atomic::value_type value ) {
  atomic_store_explicit( &var, value, std::memory_order_relaxed );
}

template < IsAtomic Atomic >
void atomic_get_with_sync( Atomic& var, typename Atomic::value_type& dstvar ) {
  dstvar = atomic_load_explicit( &var, std::memory_order_seq_cst );
}

template < IsAtomic Atomic >
void atomic_set_with_sync( Atomic& var, typename Atomic::value_type value ) {
  atomic_store_explicit( &var, value, std::memory_order_seq_cst );
}

template < IsAtomic Atomic >
void atomic_flag_get_set( Atomic&                      var,
                          typename Atomic::value_type& oldvalue_var ) {
  oldvalue_var = atomic_exchange_explicit( &var, 1, std::memory_order_relaxed );
}

#endif //! REDISCC_ATOMICVAR_H
