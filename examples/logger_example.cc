#include "log.h"
#include "logger_utils.h"
#include <thread>
#include <chrono>
#include <iostream>

using namespace rediscc::logger;

void BasicLoggingExample() {
    std::cout << "\n=== Basic Logging Example ===\n";
    
    // 基本日志记录
    REDISCC_INFO("Application started");
    REDISCC_DEBUG("Debug information: value = {}", 42);
    REDISCC_WARN("This is a warning message");
    REDISCC_ERROR("Error occurred: {}", "file not found");
    
    // 带格式的日志
    REDISCC_INFO("User {} logged in from IP {}", "john_doe", "192.168.1.100");
    
    // 条件日志
    bool debug_mode = true;
    REDISCC_LOG_IF(debug_mode, LogLevel::DEBUG, "Debug mode is enabled");
    
    // 错误码日志
    REDISCC_ERROR_CODE(404, "Resource not found: {}", "/api/users/123");
}

void ContextLoggingExample() {
    std::cout << "\n=== Context Logging Example ===\n";
    
    // 创建日志上下文
    LogContext context;
    context.request_id = "req-12345";
    context.user_id = "user-789";
    context.operation = "user_login";
    context.custom_fields["client_ip"] = "192.168.1.100";
    context.custom_fields["user_agent"] = "Mozilla/5.0";
    
    // 使用作用域上下文
    {
        REDISCC_SCOPED_CONTEXT(context);
        REDISCC_INFO("Processing user login request");
        REDISCC_DEBUG("Validating credentials");
        REDISCC_INFO("Login successful");
    }
    
    // 直接使用上下文记录日志
    REDISCC_LOG_WITH_CONTEXT(LogLevel::INFO, context, "User session created");
    
    // 审计日志
    REDISCC_AUDIT("user-789", "login", "User logged in successfully");
}

void PerformanceLoggingExample() {
    std::cout << "\n=== Performance Logging Example ===\n";
    
    // 使用性能计时器
    {
        REDISCC_PERF_TIMER("database_query");
        
        // 模拟数据库查询
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        REDISCC_INFO("Database query completed");
    } // 计时器在这里自动记录执行时间
    
    // 手动性能日志
    auto start = std::chrono::high_resolution_clock::now();
    
    // 模拟一些工作
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    REDISCC_INFO("Manual timing: operation took {} microseconds", duration.count());
}

void RequestTrackingExample() {
    std::cout << "\n=== Request Tracking Example ===\n";
    
    // 创建请求追踪器
    {
        REDISCC_REQUEST_TRACKER("req-98765");
        
        // 设置操作
        _req_tracker.SetOperation("process_order");
        _req_tracker.AddField("order_id", "order-456");
        _req_tracker.AddField("customer_id", "cust-123");
        
        // 记录里程碑
        _req_tracker.LogMilestone("validation_started");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        _req_tracker.LogMilestone("validation_completed");
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        
        _req_tracker.LogMilestone("processing_started");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // 模拟错误情况
        if (false) {
            _req_tracker.LogError("Payment processing failed");
        }
        
        _req_tracker.LogMilestone("processing_completed");
    } // 请求追踪器在这里自动记录总耗时
}

void BatchLoggingExample() {
    std::cout << "\n=== Batch Logging Example ===\n";
    
    // 创建批量日志记录器
    BatchLogger batch_logger(5); // 批次大小为5
    batch_logger.SetAutoFlushInterval(std::chrono::milliseconds(1000));
    
    // 添加多条日志到批次
    for (int i = 0; i < 12; ++i) {
        batch_logger.Add(LogLevel::INFO, "Batch log entry #{}", i + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 手动刷新剩余的日志
    batch_logger.Flush();
}

void LogLevelExample() {
    std::cout << "\n=== Log Level Example ===\n";
    
    REDISCC_INFO("Current log level messages");
    
    // 临时改变日志级别
    {
        REDISCC_LOG_LEVEL_GUARD(LogLevel::DEBUG);
        REDISCC_DEBUG("This debug message will be shown");
        REDISCC_TRACE("This trace message might be shown depending on sink config");
    }
    
    REDISCC_DEBUG("This debug message might not be shown (back to original level)");
}

void SensitiveDataExample() {
    std::cout << "\n=== Sensitive Data Filtering Example ===\n";
    
    // 这些敏感信息应该被自动过滤
    REDISCC_INFO("User login: username=john, password=secret123");
    REDISCC_INFO("API call with token=abc123xyz789");
    REDISCC_INFO("Customer phone: 13812345678");
    REDISCC_INFO("Email notification sent to user@example.com");
    REDISCC_INFO("Credit card: 1234567890123456");
}

void StatisticsExample() {
    std::cout << "\n=== Statistics Example ===\n";
    
    // 记录一些日志以生成统计信息
    for (int i = 0; i < 10; ++i) {
        REDISCC_INFO("Statistics test log #{}", i + 1);
    }
    
    REDISCC_WARN("A warning message");
    REDISCC_ERROR("An error message");
    
    // 获取并显示统计信息
    auto stats = GetLogStatistics();
    std::cout << "Total logs: " << stats.total_logs << std::endl;
    std::cout << "Average log time: " << stats.avg_log_time_ms << " ms" << std::endl;
    
    // 获取详细统计信息
    auto detailed_stats = LogStatisticsCollector::Instance().GetStatistics();
    std::cout << LogStatisticsCollector::Instance().GenerateReport() << std::endl;
}

int main() {
    std::cout << "RedisCC Logger Examples\n";
    std::cout << "=======================\n";
    
    // 初始化日志系统
    if (!InitLogger("config/logger.json")) {
        std::cerr << "Failed to initialize logger, using default config" << std::endl;
        if (!InitLogger()) {
            std::cerr << "Failed to initialize logger with default config" << std::endl;
            return 1;
        }
    }
    
    try {
        // 运行各种示例
        BasicLoggingExample();
        ContextLoggingExample();
        PerformanceLoggingExample();
        RequestTrackingExample();
        BatchLoggingExample();
        LogLevelExample();
        SensitiveDataExample();
        StatisticsExample();
        
        std::cout << "\n=== All examples completed ===\n";
        
    } catch (const std::exception& e) {
        REDISCC_ERROR("Exception in examples: {}", e.what());
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
    // 关闭日志系统
    ShutdownLogger();
    
    return 0;
}
