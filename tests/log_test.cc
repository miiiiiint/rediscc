#include "../include/log.h"
#include "../include/logger_utils.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

class LoggerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 确保日志目录存在
    std::filesystem::create_directories( "test_logs" );

    // 初始化日志系统
    ASSERT_TRUE( rediscc::logger::InitLogger() );
  }

  void TearDown() override {
    // 关闭日志系统
    rediscc::logger::ShutdownLogger();

    // 清理测试日志文件
    std::filesystem::remove_all( "test_logs" );
  }
};

TEST_F( LoggerTest, BasicLogging ) {
  // Test different log levels
  REDISCC_INFO( "This is an info message" );
  REDISCC_WARN( "This is a warning message" );
  REDISCC_ERROR( "This is an error message" );
  REDISCC_DEBUG( "This is a debug message" );
  REDISCC_TRACE( "This is a trace message" );
  REDISCC_CRITICAL( "This is a critical message" );

  // Test formatted logging
  REDISCC_INFO( "Formatted message: {} {}", 42, "test" );
  REDISCC_INFO( "Multiple params: {}, {}, {}", 1, 2.5, "string" );
}

TEST_F( LoggerTest, ContextLogging ) {
  using namespace rediscc::logger;

  // Test context logging
  LogContext context;
  context.request_id                    = "test-req-123";
  context.user_id                       = "test-user-456";
  context.operation                     = "unit_test";
  context.custom_fields[ "test_field" ] = "test_value";

  REDISCC_LOG_WITH_CONTEXT( LogLevel::INFO, context, "Context test message" );

  // Test scoped context
  {
    REDISCC_SCOPED_CONTEXT( context );
    REDISCC_INFO( "Message with scoped context" );
    REDISCC_DEBUG( "Debug message with context" );
  }

  // Test audit logging
  REDISCC_AUDIT( "test-user", "test_operation", "Audit test message" );
}

TEST_F( LoggerTest, PerformanceLogging ) {
  using namespace rediscc::logger;

  // Test performance timer
  {
    REDISCC_PERF_TIMER( "test_operation" );
    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
  }

  // Test manual performance logging
  PerformanceTimer timer( "manual_test" );
  std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
  timer.Stop();

  EXPECT_GT( timer.GetElapsedMs(), 4.0 );
}

TEST_F( LoggerTest, RequestTracking ) {
  using namespace rediscc::logger;

  {
    RequestTracker tracker( "test-req-789" );
    tracker.SetOperation( "test_operation" );
    tracker.AddField( "test_key", "test_value" );
    tracker.LogMilestone( "milestone_1" );

    std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    tracker.LogMilestone( "milestone_2" );
  }
}

TEST_F( LoggerTest, ConditionalLogging ) {
  bool condition_true  = true;
  bool condition_false = false;

  REDISCC_LOG_IF( condition_true, rediscc::logger::LogLevel::INFO, "This should be logged" );
  REDISCC_LOG_IF( condition_false, rediscc::logger::LogLevel::INFO, "This should not be logged" );
}

TEST_F( LoggerTest, ErrorCodeLogging ) {
  REDISCC_ERROR_CODE( 404, "Resource not found: {}", "/test/resource" );
  REDISCC_ERROR_CODE( 500, "Internal server error" );
}

TEST_F( LoggerTest, LogLevelGuard ) {
  using namespace rediscc::logger;

  auto original_level = LoggerManager::Instance().GetLogLevel();

  {
    LogLevelGuard guard( LogLevel::TRACE );
    EXPECT_EQ( LoggerManager::Instance().GetLogLevel(), LogLevel::TRACE );
    REDISCC_TRACE( "This trace message should be visible" );
  }

  EXPECT_EQ( LoggerManager::Instance().GetLogLevel(), original_level );
}

TEST_F( LoggerTest, BatchLogging ) {
  using namespace rediscc::logger;

  BatchLogger batch_logger( 3 );

  batch_logger.Add( LogLevel::INFO, "Batch message 1" );
  batch_logger.Add( LogLevel::INFO, "Batch message 2" );
  batch_logger.Add( LogLevel::INFO, "Batch message 3" );
  batch_logger.Add( LogLevel::INFO, "Batch message 4" );

  batch_logger.Flush();
}

TEST_F( LoggerTest, Statistics ) {
  using namespace rediscc::logger;

  // Reset statistics
  LogStatisticsCollector::Instance().ResetStatistics();

  // Generate some logs
  for ( int i = 0; i < 5; ++i ) {
    REDISCC_INFO( "Test message {}", i );
  }
  REDISCC_WARN( "Warning message" );
  REDISCC_ERROR( "Error message" );

  // Check basic statistics
  auto stats = GetLogStatistics();
  EXPECT_GT( stats.total_logs, 0 );

  // Check detailed statistics
  auto detailed_stats = LogStatisticsCollector::Instance().GetStatistics();
  EXPECT_GT( detailed_stats.total_logs, 0 );
  EXPECT_GT( detailed_stats.total_bytes, 0 );

  // Generate report
  std::string report = LogStatisticsCollector::Instance().GenerateReport();
  EXPECT_FALSE( report.empty() );
}
