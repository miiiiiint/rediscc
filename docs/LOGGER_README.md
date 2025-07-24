# RedisCC 日志系统

## 概述

RedisCC 项目采用了基于 spdlog 的高性能日志系统，提供了丰富的功能包括异步日志、结构化输出、上下文追踪、敏感信息过滤等特性。

## 主要特性

### 🚀 性能优化
- **异步日志**: 支持异步日志记录，避免阻塞主线程
- **批量处理**: 支持批量日志记录，提高高频日志场景的性能
- **缓存优化**: 避免重复获取 logger 实例
- **线程安全**: 完全线程安全的实现

### 📊 结构化日志
- **JSON 格式**: 支持 JSON 格式输出，便于日志解析和分析
- **文本格式**: 传统的文本格式，便于人工阅读
- **自定义格式**: 支持自定义日志格式

### 🔍 上下文追踪
- **请求追踪**: 支持请求 ID、会话 ID 等追踪信息
- **用户上下文**: 记录用户 ID、操作类型等信息
- **自定义字段**: 支持添加自定义上下文字段
- **线程本地存储**: 使用线程本地存储管理上下文

### 🔒 安全和隐私
- **敏感信息过滤**: 自动过滤密码、密钥等敏感信息
- **数据脱敏**: 支持邮箱、手机号等数据脱敏
- **正则表达式**: 支持自定义正则表达式过滤规则

### ⚙️ 灵活配置
- **配置文件**: 支持 JSON 配置文件
- **环境变量**: 支持环境变量配置
- **运行时配置**: 支持运行时动态调整日志级别
- **多种输出**: 支持控制台、文件、轮转文件、按日期轮转等多种输出方式

## 快速开始

### 1. 初始化日志系统

```cpp
#include "log.h"

int main() {
    // 使用默认配置初始化
    if (!rediscc::logger::InitLogger()) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return 1;
    }
    
    // 或者使用配置文件初始化
    if (!rediscc::logger::InitLogger("config/logger.json")) {
        std::cerr << "Failed to initialize logger with config" << std::endl;
        return 1;
    }
    
    // 程序结束时关闭日志系统
    rediscc::logger::ShutdownLogger();
    return 0;
}
```

### 2. 基本日志记录

```cpp
// 基本日志级别
REDISCC_TRACE("Trace message");
REDISCC_DEBUG("Debug message");
REDISCC_INFO("Info message");
REDISCC_WARN("Warning message");
REDISCC_ERROR("Error message");
REDISCC_CRITICAL("Critical message");

// 格式化日志
REDISCC_INFO("User {} logged in from IP {}", "john_doe", "192.168.1.100");
REDISCC_ERROR("Failed to process order {}: {}", order_id, error_msg);
```

### 3. 上下文日志

```cpp
#include "logger_utils.h"

// 创建日志上下文
rediscc::logger::LogContext context;
context.request_id = "req-12345";
context.user_id = "user-789";
context.operation = "user_login";

// 使用作用域上下文
{
    REDISCC_SCOPED_CONTEXT(context);
    REDISCC_INFO("Processing user login request");
    REDISCC_DEBUG("Validating credentials");
}

// 直接使用上下文
REDISCC_LOG_WITH_CONTEXT(rediscc::logger::LogLevel::INFO, context, "Login successful");

// 审计日志
REDISCC_AUDIT("user-789", "login", "User logged in successfully");
```

### 4. 性能日志

```cpp
// 自动性能计时
{
    REDISCC_PERF_TIMER("database_query");
    // 执行数据库查询
    query_database();
} // 自动记录执行时间

// 手动性能计时
rediscc::logger::PerformanceTimer timer("complex_operation");
perform_complex_operation();
timer.Stop(); // 手动停止并记录
```

### 5. 请求追踪

```cpp
{
    REDISCC_REQUEST_TRACKER("req-98765");
    
    _req_tracker.SetOperation("process_order");
    _req_tracker.AddField("order_id", "order-456");
    
    _req_tracker.LogMilestone("validation_started");
    validate_order();
    
    _req_tracker.LogMilestone("processing_started");
    process_order();
    
    _req_tracker.LogMilestone("completed");
} // 自动记录总耗时
```

## 配置说明

### 配置文件格式

```json
{
  "logger_name": "REDISCC",
  "level": "info",
  "format": "text",
  "auto_flush": true,
  "flush_interval_ms": 1000,
  
  "async": {
    "enabled": true,
    "queue_size": 8192,
    "thread_count": 1
  },
  
  "context": {
    "include_thread_id": true,
    "include_process_id": false,
    "include_timestamp": true
  },
  
  "filter": {
    "enabled": true,
    "sensitive_keys": ["password", "token", "secret"],
    "mask_string": "***"
  },
  
  "sinks": [
    {
      "type": "console",
      "pattern": "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v",
      "level": "info",
      "colored": true
    },
    {
      "type": "rotating_file",
      "pattern": "[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v",
      "level": "debug",
      "filename": "logs/rediscc.log",
      "max_size": 10485760,
      "max_files": 5
    }
  ]
}
```

### 环境变量配置

```bash
export REDISCC_LOG_LEVEL=debug
export REDISCC_LOG_FORMAT=json
export REDISCC_LOG_ASYNC=true
export REDISCC_LOG_FILE=logs/app.log
```

## 最佳实践

### 1. 日志级别使用指南

- **TRACE**: 最详细的调试信息，通常只在开发阶段使用
- **DEBUG**: 调试信息，用于问题诊断
- **INFO**: 一般信息，记录程序的正常运行状态
- **WARN**: 警告信息，表示潜在问题但不影响程序运行
- **ERROR**: 错误信息，表示程序遇到错误但可以继续运行
- **CRITICAL**: 严重错误，可能导致程序无法继续运行

### 2. 性能考虑

- 在高频调用的代码路径中使用异步日志
- 避免在日志消息中进行复杂的字符串操作
- 使用批量日志记录器处理大量日志
- 合理设置日志级别，避免不必要的日志输出

### 3. 安全考虑

- 启用敏感信息过滤
- 定期审查日志内容，确保没有泄露敏感信息
- 设置适当的日志文件权限
- 在生产环境中使用较高的日志级别

### 4. 监控和维护

- 定期检查日志文件大小和磁盘使用情况
- 监控日志系统的性能指标
- 设置日志轮转策略，避免日志文件过大
- 建立日志分析和告警机制

## 故障排除

### 常见问题

1. **日志文件无法创建**
   - 检查日志目录是否存在
   - 检查文件权限
   - 确保磁盘空间充足

2. **日志级别不生效**
   - 检查配置文件格式
   - 确认环境变量设置
   - 验证 sink 的级别配置

3. **性能问题**
   - 启用异步日志
   - 调整队列大小和线程数
   - 减少不必要的日志输出

4. **敏感信息泄露**
   - 检查过滤器配置
   - 添加自定义过滤规则
   - 审查日志内容

## 示例代码

完整的使用示例请参考 `examples/logger_example.cc` 文件。

## API 参考

详细的 API 文档请参考头文件中的注释：
- `include/log.h` - 主要日志接口
- `include/logger_manager.h` - 日志管理器
- `include/logger_config.h` - 配置管理
- `include/logger_utils.h` - 工具类
