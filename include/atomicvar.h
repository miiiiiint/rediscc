#ifndef REDISCC_ATOMICVAR_H
#define REDISCC_ATOMICVAR_H

#include <concepts>
#include <type_traits>
#ifdef __has_include
#  if __has_include( <atomic> )
#    include <atomic>
#  endif
#else
#  error "must support <atomic>"
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

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地增加变量的值
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param [out] var 要增加的原子变量
 * @param [in] count 要增加的值
 */
template < IsAtomic Atomic >
void atomic_inc( Atomic& var, typename Atomic::value_type count ) {
  atomic_fetch_add_explicit( &var, count, std::memory_order_relaxed );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地增加变量的值，并返回增加前的值
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要增加的原子变量
 * @param [out] oldvalue_var 增加前的值
 * @param [in] count 要增加的值
 */
template < IsAtomic Atomic >
void atomic_get_inc( Atomic&                      var,
                     typename Atomic::value_type& oldvalue_var,
                     typename Atomic::value_type  count ) {
  oldvalue_var = atomic_fetch_add_explicit( &var, count, std::memory_order_relaxed );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地增加变量的值，并返回增加后的值
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要增加的原子变量
 * @param [out] newvalue_var 增加后的值
 * @param [in] count 要增加的值
 */
template < IsAtomic Atomic >
void atomic_inc_get( Atomic&                      var,
                     typename Atomic::value_type& newvalue_var,
                     typename Atomic::value_type  count ) {
  newvalue_var = atomic_fetch_add_explicit( &var, count, std::memory_order_relaxed ) + count;
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地减少变量的值
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要减少的原子变量
 * @param count 要减少的值
 */
template < IsAtomic Atomic >
void atomic_dec( Atomic& var, typename Atomic::value_type count ) {
  atomic_fetch_sub_explicit( &var, count, std::memory_order_relaxed );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地获取变量的值
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要获取值的原子变量
 * @param [out] dstvar 获取到的值
 */
template < IsAtomic Atomic >
void atomic_get( Atomic& var, typename Atomic::value_type& dstvar ) {
  dstvar = atomic_load_explicit( &var, std::memory_order_relaxed );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地设置变量的值
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要设置的原子变量
 * @param value 要设置的值
 */
template < IsAtomic Atomic >
void atomic_set( Atomic& var, typename Atomic::value_type value ) {
  atomic_store_explicit( &var, value, std::memory_order_relaxed );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地获取变量的值，使用顺序一致性内存序
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要获取值的原子变量
 * @param [out] dstvar 获取到的值
 */
template < IsAtomic Atomic >
void atomic_get_with_sync( Atomic& var, typename Atomic::value_type& dstvar ) {
  dstvar = atomic_load_explicit( &var, std::memory_order_seq_cst );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地设置变量的值，使用顺序一致性内存序
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要设置的原子变量
 * @param value 要设置的值
 */
template < IsAtomic Atomic >
void atomic_set_with_sync( Atomic& var, typename Atomic::value_type value ) {
  atomic_store_explicit( &var, value, std::memory_order_seq_cst );
}

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-11
 * @brief 原子地获取并设置变量的值
 *
 * @tparam Atomic 满足IsAtomic概念的原子类型
 * @param var 要操作的原子变量
 * @param [out] oldvalue_var 获取到的旧值
 */
template < IsAtomic Atomic >
void atomic_flag_get_set( Atomic& var, typename Atomic::value_type& oldvalue_var ) {
  oldvalue_var = atomic_exchange_explicit( &var, 1, std::memory_order_relaxed );
}

#endif  //! REDISCC_ATOMICVAR_H
