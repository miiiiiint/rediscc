# RedisCC 

## 1. 日志级别使用规范

### 级别定义和使用场景

| 级别 | 使用场景 | 示例 |
|------|----------|------|
| TRACE | 最详细的执行流程，仅用于深度调试 | 函数进入/退出、循环迭代 |
| DEBUG | 调试信息，帮助开发者理解程序行为 | 变量值、条件判断结果 |
| INFO | 重要的业务事件和系统状态 | 用户登录、订单创建、服务启动 |
| WARN | 潜在问题，不影响正常功能 | 配置缺失使用默认值、重试操作 |
| ERROR | 错误事件，但程序可以继续运行 | 网络请求失败、数据验证错误 |
| CRITICAL | 严重错误，可能导致程序崩溃 | 数据库连接失败、内存不足 |

### 级别选择原则

```cpp
// ✅ 正确使用
REDISCC_INFO("User {} successfully logged in", user_id);
REDISCC_WARN("Database connection pool is 90% full");
REDISCC_ERROR("Failed to send email to {}: {}", email, error_msg);

// ❌ 错误使用
REDISCC_ERROR("User clicked button");  // 应该用 DEBUG 或 TRACE
REDISCC_INFO("Variable x = {}", x);    // 应该用 DEBUG
```

## 2. 性能优化指南

### 2.1 异步日志配置

```json
{
  "async": {
    "enabled": true,
    "queue_size": 8192,      // 根据日志量调整
    "thread_count": 1,       // 通常1个线程足够
    "overflow_policy_block": false  // 避免阻塞主线程
  }
}
```

### 2.2 高频日志优化

```cpp
// ✅ 使用批量日志记录器
rediscc::logger::BatchLogger batch_logger(100);
for (int i = 0; i < 10000; ++i) {
    batch_logger.Add(LogLevel::DEBUG, "Processing item {}", i);
}

// ✅ 条件日志记录
REDISCC_LOG_IF(debug_enabled, LogLevel::DEBUG, "Debug info: {}", data);

// ❌ 避免在循环中直接记录大量日志
for (int i = 0; i < 10000; ++i) {
    REDISCC_DEBUG("Processing item {}", i);  // 性能问题
}
```

### 2.3 字符串格式化优化

```cpp
// ✅ 使用 fmt 格式化
REDISCC_INFO("User {} has {} items", user_id, item_count);

// ❌ 避免复杂的字符串操作
std::string msg = "User " + user_id + " has " + std::to_string(item_count) + " items";
REDISCC_INFO(msg);
```

## 3. 上下文管理最佳实践

### 3.1 请求级别上下文

```cpp
void HandleRequest(const HttpRequest& request) {
    // 为整个请求设置上下文
    REDISCC_REQUEST_TRACKER(request.GetId());
    _req_tracker.AddField("user_id", request.GetUserId());
    _req_tracker.AddField("endpoint", request.GetPath());
    
    try {
        _req_tracker.LogMilestone("validation_start");
        ValidateRequest(request);
        
        _req_tracker.LogMilestone("processing_start");
        ProcessRequest(request);
        
        _req_tracker.LogMilestone("completed");
    } catch (const std::exception& e) {
        _req_tracker.LogError(e.what());
        throw;
    }
}
```

### 3.2 作用域上下文

```cpp
void ProcessOrder(const Order& order) {
    rediscc::logger::LogContext context;
    context.operation = "process_order";
    context.custom_fields["order_id"] = order.GetId();
    context.custom_fields["customer_id"] = order.GetCustomerId();
    
    REDISCC_SCOPED_CONTEXT(context);
    
    REDISCC_INFO("Starting order processing");
    // 所有后续日志都会包含上下文信息
    ValidateOrder(order);
    ChargePayment(order);
    ShipOrder(order);
    REDISCC_INFO("Order processing completed");
}
```

## 4. 安全和隐私保护

### 4.1 敏感信息过滤配置

```json
{
  "filter": {
    "enabled": true,
    "sensitive_keys": [
      "password", "passwd", "pwd", "secret", "key", "token", "auth",
      "authorization", "credential", "private_key", "access_token",
      "session_id", "cookie", "jwt", "api_key", "client_secret"
    ],
    "sensitive_patterns": [
      "\\b\\d{11}\\b",                    // 手机号
      "\\b[\\w\\.-]+@[\\w\\.-]+\\.\\w+\\b", // 邮箱
      "\\b\\d{15,19}\\b",                 // 银行卡号
      "\\b[A-Za-z0-9+/]{40,}={0,2}\\b"   // Base64编码的密钥
    ],
    "mask_string": "***MASKED***"
  }
}
```

### 4.2 安全日志记录

```cpp
// ✅ 安全的日志记录
REDISCC_INFO("User authentication attempt for user: {}", username);
REDISCC_AUDIT(user_id, "login", "User logged in from IP: {}", client_ip);

// ❌ 避免记录敏感信息
REDISCC_INFO("User login: username={}, password={}", username, password);
REDISCC_DEBUG("API request: {}", request_with_token);
```

## 5. 生产环境配置

### 5.1 生产环境日志配置

```json
{
  "level": "warn",           // 生产环境使用较高级别
  "format": "json",          // JSON格式便于日志分析
  "auto_flush": true,
  "flush_interval_ms": 5000, // 较长的刷新间隔
  
  "async": {
    "enabled": true,
    "queue_size": 16384,     // 更大的队列
    "thread_count": 2        // 多线程处理
  },
  
  "sinks": [
    {
      "type": "daily_file",
      "level": "warn",
      "filename": "logs/app_%Y%m%d.log"
    },
    {
      "type": "rotating_file",
      "level": "error",
      "filename": "logs/error.log",
      "max_size": 50485760,  // 50MB
      "max_files": 10
    }
  ]
}
```

### 5.2 监控和告警

```cpp
// 定期检查日志统计
void CheckLogHealth() {
    auto stats = rediscc::logger::GetLogStatistics();
    
    if (stats.error_count > ERROR_THRESHOLD) {
        // 发送告警
        SendAlert("High error rate detected");
    }
    
    if (stats.avg_log_time_ms > PERFORMANCE_THRESHOLD) {
        // 性能告警
        SendAlert("Log performance degradation");
    }
}
```

## 6. 错误处理和故障排除

### 6.1 错误分类和处理

```cpp
// 业务错误
REDISCC_ERROR_CODE(1001, "Invalid user input: {}", input);

// 系统错误
REDISCC_ERROR_CODE(2001, "Database connection failed: {}", db_error);

// 外部服务错误
REDISCC_ERROR_CODE(3001, "External API call failed: {} - {}", api_name, error);
```

### 6.2 异常日志记录

```cpp
try {
    RiskyOperation();
} catch (const BusinessException& e) {
    REDISCC_WARN("Business rule violation: {}", e.what());
    // 继续处理
} catch (const SystemException& e) {
    REDISCC_ERROR("System error occurred: {}", e.what());
    // 错误恢复
} catch (const std::exception& e) {
    REDISCC_CRITICAL("Unexpected error: {}", e.what());
    // 可能需要终止程序
    throw;
}
```

## 7. 测试和调试

### 7.1 测试环境配置

```json
{
  "level": "debug",
  "format": "text",
  "sinks": [
    {
      "type": "console",
      "level": "debug",
      "colored": true
    },
    {
      "type": "file",
      "level": "trace",
      "filename": "test_logs/test.log"
    }
  ]
}
```

### 7.2 调试技巧

```cpp
// 临时提高日志级别
{
    REDISCC_LOG_LEVEL_GUARD(LogLevel::TRACE);
    
    REDISCC_TRACE("Entering complex function");
    ComplexFunction();
    REDISCC_TRACE("Exiting complex function");
}

// 性能分析
{
    REDISCC_PERF_TIMER("critical_section");
    CriticalSection();
}
```

## 8. 日志分析和维护

### 8.1 日志轮转策略

```json
{
  "sinks": [
    {
      "type": "daily_file",
      "filename": "logs/app_%Y%m%d.log",
      "rotation_hour": 0,
      "rotation_minute": 0
    },
    {
      "type": "rotating_file",
      "filename": "logs/app.log",
      "max_size": 104857600,  // 100MB
      "max_files": 7          // 保留7个文件
    }
  ]
}
```

### 8.2 日志清理脚本

```bash
#!/bin/bash
# 清理30天前的日志文件
find /path/to/logs -name "*.log" -mtime +30 -delete

# 压缩7天前的日志文件
find /path/to/logs -name "*.log" -mtime +7 -exec gzip {} \;
```

## 9. 常见反模式

### 9.1 避免的做法

```cpp
// ❌ 日志级别滥用
REDISCC_ERROR("User clicked button");

// ❌ 过度日志记录
for (auto& item : large_collection) {
    REDISCC_DEBUG("Processing item: {}", item.ToString());
}

// ❌ 敏感信息泄露
REDISCC_INFO("Login attempt: {}", login_request.ToJson());

// ❌ 性能影响
REDISCC_DEBUG("Complex calculation result: {}", ExpensiveCalculation());

// ❌ 无意义的日志
REDISCC_INFO("Function called");
```

### 9.2 推荐的做法

```cpp
// ✅ 合适的日志级别
REDISCC_DEBUG("User interaction: button clicked");

// ✅ 聚合日志
REDISCC_INFO("Processed {} items in {} ms", count, duration);

// ✅ 安全的日志记录
REDISCC_INFO("Login attempt for user: {}", username);

// ✅ 条件性能日志
if (logger_manager.GetLogLevel() <= LogLevel::DEBUG) {
    REDISCC_DEBUG("Calculation result: {}", ExpensiveCalculation());
}

// ✅ 有意义的日志
REDISCC_INFO("User authentication completed successfully");
```

## 10. 总结

遵循这些最佳实践可以帮助您：

1. **提高性能**: 通过异步日志和批量处理减少性能影响
2. **增强安全性**: 通过敏感信息过滤保护用户隐私
3. **便于调试**: 通过结构化日志和上下文信息快速定位问题
4. **简化维护**: 通过合理的配置和轮转策略减少维护工作
5. **支持监控**: 通过统计信息和告警机制及时发现问题

记住，好的日志系统不仅要记录信息，更要帮助开发者和运维人员快速理解系统状态和解决问题。
