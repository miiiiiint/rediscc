#include "../include/log.h"
#include <gtest/gtest.h>

TEST( LogTest, BasicLogging ) {
  // Test different log levels
  REDISCC_INFO( "This is an info message" );
  REDISCC_WARN( "This is a warning message" );
  REDISCC_ERROR( "This is an error message" );

  // Test formatted logging
  REDISCC_INFO( "Formatted message: %d %s", 42, "test" );

  // Test debug logging
  REDISCC_DEBUG( "Debug message: {}", "test" );
}
