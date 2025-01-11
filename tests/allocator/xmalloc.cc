#include <allocator/xmalloc.h>
#include <gtest/gtest.h>

TEST( XMALLOC, simple_malloc_free ) {
  char* ptr = static_cast< char* >( xmalloc( 5 ) );
  ASSERT_NE( ptr, nullptr );

  xfree( ptr );
}

TEST( XMALLOC, AllocateValidMemory ) {
  size_t size = 128;
  void*  ptr  = xmalloc( size );

  ASSERT_NE( ptr, nullptr ) << "xmalloc failed to allocate memory";
  EXPECT_EQ( xmalloc_size( ptr ), size ) << "Allocated size does not match requested size";

  xfree( ptr );  // Ensure it can be freed without errors
}

TEST( XMALLOC, AllocateZeroBytes ) {
  size_t size = 0;
  void*  ptr  = xmalloc( size );

  // Behavior may vary based on implementation; check if a non-null pointer is returned
  EXPECT_NE( ptr, nullptr ) << "xmalloc failed to handle zero-byte allocation";
  EXPECT_EQ( xmalloc_size( ptr ), sizeof( int64_t ) ) << "xmalloc should allocate zero bytes";

  xfree( ptr );  // Ensure it can be freed safely
}

TEST( XMALLOC, FreeNullPointer ) {
  void* ptr = nullptr;

  // Should handle null pointers gracefully without any errors or crashes
  ASSERT_NO_THROW( xfree( ptr ) ) << "xfree should handle null pointers safely";
}

TEST( XMALLOC, AllocateAndWriteMemory ) {
  size_t size = 64;
  void*  ptr  = xmalloc( size );

  ASSERT_NE( ptr, nullptr ) << "xmalloc failed to allocate memory";

  // Write to the allocated memory and verify correctness
  memset( ptr, 0xAB, size );
  uint8_t* bytes = static_cast< uint8_t* >( ptr );
  for ( size_t i = 0; i < size; ++i ) {
    EXPECT_EQ( bytes[ i ], 0xAB ) << "Memory write/read mismatch at index " << i;
  }

  xfree( ptr );
}

TEST( XMALLOC, RepeatedAllocations ) {
  constexpr int    num_allocations = 1000;
  constexpr size_t size            = 32;

  std::vector< void* > allocations;
  for ( int i = 0; i < num_allocations; ++i ) {
    void* ptr = xmalloc( size );
    ASSERT_NE( ptr, nullptr ) << "xmalloc failed on allocation " << i;
    allocations.push_back( ptr );
  }

  for ( void* ptr : allocations ) {
    xfree( ptr );  // Free all allocated memory
  }
}

// TEST( XMALLOC, OutOfMemoryHandler ) {
//   // Simulate a scenario where memory allocation fails
//   // Assume `xmalloc_oom_handler` logs an error or aborts the program.
//   // We cannot directly simulate `malloc` failing in unit tests unless mocking is used.
//   size_t very_large_size =
//     static_cast< size_t >( -1 ); // Request a very large size
//   EXPECT_ANY_THROW( {
//     void* ptr = xmalloc( very_large_size );
//     xfree( ptr );
//   } )
//     << "xmalloc should trigger OOM handler on excessive allocation request";
// }

TEST( XMALLOC, FreeMultipleTimes ) {
  size_t size = 64;
  void*  ptr  = xmalloc( size );

  ASSERT_NE( ptr, nullptr ) << "xmalloc failed to allocate memory";

  xfree( ptr );  // Free once
  ASSERT_NO_THROW( xfree( ptr ) ) << "xfree should handle double-free gracefully";
}

TEST( XMALLOC, AllocateLargeMemoryBlock ) {
  size_t large_size = 1024 * 1024 * 10;  // 10 MB
  void*  ptr        = xmalloc( large_size );

  ASSERT_NE( ptr, nullptr ) << "xmalloc failed to allocate large memory block";

  // Optional: Test writing to the large memory block
  memset( ptr, 0xCD, large_size );

  xfree( ptr );
}

TEST( XMALLOC, ReallocateMemory ) {
  size_t initial_size = 128;
  void*  ptr          = xmalloc( initial_size );
  ASSERT_NE( ptr, nullptr ) << "xmalloc failed to allocate initial memory";

  size_t new_size = 256;
  ptr             = xrealloc( ptr, new_size );
  ASSERT_NE( ptr, nullptr ) << "xrealloc failed to reallocate memory";
  EXPECT_GE( xmalloc_size( ptr ), new_size ) << "Reallocated memory size is less than requested";

  xfree( ptr );
}

TEST( XMALLOC, ReallocateNullPointer ) {
  // Reallocating a null pointer should behave like malloc
  size_t size = 64;
  void*  ptr  = xrealloc( nullptr, size );

  ASSERT_NE( ptr, nullptr ) << "xrealloc failed to allocate memory for null pointer";
  EXPECT_GE( xmalloc_size( ptr ), size ) << "Allocated memory size is less than requested";

  xfree( ptr );
}

TEST( XMALLOC, ReallocateToZero ) {
  size_t size = 128;
  void*  ptr  = xmalloc( size );
  ASSERT_NE( ptr, nullptr ) << "xmalloc failed to allocate memory";

  // Reallocating to size 0 should free the memory and return nullptr
  void* new_ptr = xrealloc( ptr, 0 );
  EXPECT_EQ( new_ptr, nullptr ) << "xrealloc should return nullptr when reallocating to zero size";
}

TEST( XMALLOC, TryXmallocUsableMemory ) {
  size_t size        = 128;
  size_t usable_size = 0;
  void*  ptr         = try_xmalloc_usable( size, &usable_size );

  ASSERT_NE( ptr, nullptr ) << "try_xmalloc_usable failed to allocate memory";
  EXPECT_GE( usable_size, size ) << "Usable memory size is less than requested";

  xfree( ptr );
}

TEST( XMALLOC, ExtendToUsable ) {
  size_t size = 128;
  void*  ptr  = xmalloc( size );

  void* extended_ptr = extend_to_usable( ptr, size );
  EXPECT_EQ( ptr, extended_ptr ) << "extend_to_usable should return the same pointer";

  xfree( ptr );
}

TEST( XMALLOC, MultiThreadedAllocations ) {
  constexpr int    num_threads            = 10;
  constexpr int    allocations_per_thread = 100;
  constexpr size_t size                   = 64;

  auto worker = []() {
    for ( int i = 0; i < allocations_per_thread; ++i ) {
      void* ptr = xmalloc( size );
      ASSERT_NE( ptr, nullptr ) << "xmalloc failed in multi-threaded allocation";
      xfree( ptr );
    }
  };

  std::vector< std::thread > threads;
  for ( int i = 0; i < num_threads; ++i ) {
    threads.emplace_back( worker );
  }

  for ( auto& thread : threads ) {
    thread.join();
  }
}

GTEST_API_ int main( int argc, char* argv[] ) {
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
