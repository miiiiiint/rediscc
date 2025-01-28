/**
 * @file template_doxygen.cc
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-01-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <cstdint>

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-14
 * @brief
 *
 */
class template_doxygen {
private:
  [[maybe_unused]]
  int a_;     ///<
  [[maybe_unused]]
  double d_;  ///<

public:
  /**
   * @author chenmiao (chenmiao.ku@gmail.com)
   * @date 2025-01-14
   * @brief Constructor
   *
   */
  template_doxygen() = default;

  /**
   * @author chenmiao (chenmiao.ku@gmail.com)
   * @date 2025-01-14
   * @brief Constructor
   *
   * @param [in] a
   * @param [in] b
   */
  template_doxygen( int a, int b );

  /**
   * @author chenmiao (chenmiao.ku@gmail.com)
   * @date 2025-01-14
   * @brief Destructor
   *
   */
  ~template_doxygen() = default;

  /**
   * @author chenmiao (chenmiao.ku@gmail.com)
   * @date 2025-01-14
   * @brief
   *
   * @param [in] a
   * @param [out] b
   * @return int32_t
   */
  int32_t test( [[maybe_unused]] int a, [[maybe_unused]] char* b ) { return 0; }
};

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-14
 * @brief
 *
 * @param [in] a
 * @param [out] b
 * @return int32_t
 */
int32_t simple_func( [[maybe_unused]] int a, [[maybe_unused]] char* b ) { return 0; }

/**
 * @author chenmiao (chenmiao.ku@gmail.com)
 * @date 2025-01-14
 * @brief
 *
 * @tparam T
 * @param a
 * @return T
 */
template < typename T >
T simple_template_func( T a ) {
  return a;
}
