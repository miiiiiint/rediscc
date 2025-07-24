# 🔧 代码格式化快速指南

## 快速开始

### 1️⃣ 一键设置开发环境
```bash
# 安装所有工具并配置开发环境
./scripts/setup_dev_env.sh
```

### 2️⃣ 日常使用
```bash
# 检查代码格式
./check.sh

# 自动修复格式问题
./format.sh
```

### 3️⃣ Git 提交
```bash
# 正常提交（会自动检查格式）
git add .
git commit -m "your commit message"

# 如果格式检查失败，运行修复后重新提交
./format.sh
git add .
git commit -m "your commit message"
```

## 🛠️ 支持的工具

| 工具 | 文件类型 | 作用 |
|------|----------|------|
| **clang-format** | C++ 文件 | 代码格式化 |
| **clang-tidy** | C++ 文件 | 静态分析 |
| **cmake-format** | CMake 文件 | CMake 格式化 |
| **prettier** | JSON/YAML/MD | 文档格式化 |
| **shellcheck** | Shell 脚本 | 脚本检查 |

## 📋 编码规范要点

### C++ 代码
- **缩进**: 2 个空格
- **行宽**: 最大 100 字符
- **命名**: 类名 `PascalCase`，变量 `snake_case`
- **大括号**: Attach 风格

### 示例
```cpp
class LoggerManager {
public:
    bool Initialize();
    
private:
    std::string logger_name_;
    bool initialized_;
};

namespace rediscc {
namespace logger {

void InitLogger() {
    if (condition) {
        DoSomething();
    }
}

} // namespace logger
} // namespace rediscc
```

## 🚀 高级用法

### 增量检查（仅检查变更文件）
```bash
./scripts/format_check.sh --incremental
./scripts/format_fix.sh --incremental
```

### 指定工具
```bash
# 只运行 clang-format
./scripts/format_check.sh --tools clang-format

# 运行多个工具
./scripts/format_check.sh --tools clang-format,prettier
```

### 指定文件
```bash
./scripts/format_check.sh --files "src/main.cpp,include/log.h"
```

## 🔧 IDE 配置

### VSCode
项目已包含 `.vscode/settings.json` 配置，支持：
- 保存时自动格式化
- clang-format 集成
- prettier 集成

### 其他 IDE
参考 `.editorconfig` 文件配置编辑器设置。

## 🤖 CI/CD 集成

### 自动检查
- **Push/PR**: 自动运行格式检查
- **增量检查**: PR 只检查变更文件
- **报告生成**: 自动生成格式检查报告

### 自动修复
在 PR 评论中输入 `/format-fix` 触发自动修复。

## ❓ 常见问题

### Q: 工具安装失败？
```bash
# 重新安装
./scripts/setup_dev_env.sh --tools-only

# 手动安装（Ubuntu/Debian）
sudo apt-get install clang-format-18 clang-tidy-18 shellcheck
pip3 install cmake-format
npm install -g prettier
```

### Q: 格式检查失败？
```bash
# 查看详细错误
./scripts/format_check.sh --verbose

# 自动修复
./format.sh
```

### Q: pre-commit hooks 不工作？
```bash
# 重新安装
pre-commit uninstall
pre-commit install
```

### Q: 跳过格式检查？
```bash
# 不推荐，但可以跳过
git commit --no-verify
```

## 📚 更多信息

- 📖 [完整格式化指南](docs/FORMAT_GUIDE.md)
- 🔧 [工具配置说明](docs/FORMAT_GUIDE.md#工具配置详解)
- 🎯 [最佳实践](docs/FORMAT_GUIDE.md#最佳实践)

## 🆘 获取帮助

```bash
# 查看脚本帮助
./scripts/format_check.sh --help
./scripts/format_fix.sh --help
./scripts/setup_dev_env.sh --help

# 查看工具版本
clang-format --version
cmake-format --version
prettier --version
```

