# RedisCC 代码格式化指南

## 概述

RedisCC 项目采用自动化的代码格式检查和修复系统，确保代码风格的一致性和质量。本指南介绍如何使用格式化工具和遵循项目的编码规范。

## 支持的工具

### 🔧 格式化工具

| 工具 | 用途 | 文件类型 | 配置文件 |
|------|------|----------|----------|
| **clang-format** | C++ 代码格式化 | `.cpp`, `.cc`, `.h`, `.hpp` 等 | `.clang-format` |
| **clang-tidy** | C++ 静态分析 | `.cpp`, `.cc`, `.cxx` | `.clang-tidy` |
| **cmake-format** | CMake 文件格式化 | `CMakeLists.txt`, `.cmake` | `.cmake-format.yaml` |
| **prettier** | 文档格式化 | `.json`, `.yaml`, `.md` | `.prettierrc` |
| **shellcheck** | Shell 脚本检查 | `.sh` | 内置规则 |

### 📝 编辑器配置

- **EditorConfig**: `.editorconfig` - 统一编辑器设置
- **VSCode**: `.vscode/settings.json` - VSCode 特定配置

## 快速开始

### 1. 环境设置

```bash
# 自动安装所有工具和配置开发环境
./scripts/setup_dev_env.sh

# 或者只安装工具
./scripts/setup_dev_env.sh --tools-only

# 或者只设置 pre-commit hooks
./scripts/setup_dev_env.sh --hooks-only
```

### 2. 日常使用

```bash
# 检查代码格式（推荐在提交前运行）
./check.sh

# 自动修复格式问题
./format.sh

# 检查特定工具
./scripts/format_check.sh --tools clang-format,prettier

# 修复特定文件
./scripts/format_fix.sh --files "src/main.cpp,include/log.h"
```

### 3. Git 集成

```bash
# 安装 pre-commit hooks（自动在提交时检查）
pre-commit install

# 手动运行 pre-commit 检查
pre-commit run --all-files

# 跳过 pre-commit 检查（不推荐）
git commit --no-verify
```

## 编码规范

### C++ 代码规范

#### 基本格式
- **缩进**: 2 个空格，不使用 Tab
- **行宽**: 最大 100 字符
- **换行**: Unix 风格 (LF)
- **文件结尾**: 必须有换行符

#### 命名规范
```cpp
// 类名：PascalCase
class LoggerManager {
public:
    // 公共方法：PascalCase
    bool Initialize();
    void SetLogLevel(LogLevel level);
    
private:
    // 私有成员：snake_case + 下划线后缀
    std::string logger_name_;
    bool initialized_;
    
    // 私有方法：PascalCase
    void CreateSinks();
};

// 命名空间：snake_case
namespace rediscc {
namespace logger {

// 函数：PascalCase
void InitLogger();

// 变量：snake_case
int max_file_size = 1024;

// 常量：UPPER_CASE
const int MAX_LOG_LEVEL = 5;

// 枚举：PascalCase，枚举值：UPPER_CASE
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2
};

} // namespace logger
} // namespace rediscc
```

#### 代码风格
```cpp
// 大括号：Attach 风格
if (condition) {
    DoSomething();
} else {
    DoSomethingElse();
}

// 函数参数：每行一个（如果太长）
void LongFunctionName(
    const std::string& first_parameter,
    int second_parameter,
    bool third_parameter) {
    // 函数体
}

// 指针和引用：左对齐
int* ptr = nullptr;
const std::string& ref = str;

// 模板：空格包围尖括号
std::vector< int > numbers;
std::map< std::string, int > mapping;

// 初始化列表：逗号前换行
Constructor()
    : member1_(value1)
    , member2_(value2)
    , member3_(value3) {
}
```

### CMake 规范

```cmake
# 命令：小写
cmake_minimum_required(VERSION 3.5.0)
project(rediscc VERSION 0.1.0 LANGUAGES C CXX)

# 变量：UPPER_CASE
set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

# 缩进：2 个空格
if(ENABLE_TESTING)
  add_subdirectory(tests)
endif()

# 函数调用：参数对齐
target_link_libraries(rediscc
  PRIVATE
    spdlog::spdlog
    ${JEMALLOC_LIBRARY}
)
```

### JSON/YAML 规范

```json
{
  "name": "rediscc",
  "version": "1.0.0",
  "scripts": {
    "format": "prettier --write .",
    "check": "prettier --check ."
  }
}
```

```yaml
# YAML: 2 空格缩进
name: CI
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Run tests
        run: make test
```

## 工具配置详解

### clang-format 配置

主要配置项（`.clang-format`）：
```yaml
BasedOnStyle: LLVM
ColumnLimit: 100
IndentWidth: 2
UseTab: Never
PointerAlignment: Left
ReferenceAlignment: Left
NamespaceIndentation: All
```

### clang-tidy 配置

启用的检查类别（`.clang-tidy`）：
- `bugprone-*`: 潜在 bug 检查
- `performance-*`: 性能问题检查
- `readability-*`: 可读性检查
- `modernize-*`: 现代 C++ 特性建议
- `cppcoreguidelines-*`: C++ 核心指南检查

### pre-commit 配置

自动运行的检查（`.pre-commit-config.yaml`）：
- 格式化检查
- 尾随空格清理
- 文件结尾换行符
- YAML/JSON 语法检查
- 大文件检查

## IDE 集成

### Visual Studio Code

推荐扩展：
- **C/C++**: Microsoft C++ 扩展
- **clangd**: LLVM clangd 语言服务器
- **CMake Tools**: CMake 支持
- **Prettier**: 代码格式化
- **ShellCheck**: Shell 脚本检查

配置文件（`.vscode/settings.json`）：
```json
{
  "editor.formatOnSave": true,
  "C_Cpp.clang_format_style": "file",
  "prettier.configPath": ".prettierrc"
}
```

### CLion

1. 导入 `.clang-format` 配置：
   - Settings → Editor → Code Style → C/C++
   - 选择 "Set from..." → "Predefined Style" → "File"

2. 启用 clang-tidy：
   - Settings → Tools → External Tools
   - 添加 clang-tidy 工具

### Vim/Neovim

使用 ALE 插件：
```vim
let g:ale_linters = {
\   'cpp': ['clang', 'clangtidy'],
\   'cmake': ['cmakelint'],
\}

let g:ale_fixers = {
\   'cpp': ['clang-format'],
\   'cmake': ['cmakeformat'],
\   'json': ['prettier'],
\}
```

## CI/CD 集成

### GitHub Actions

项目包含三个主要工作流：

1. **格式检查** (`.github/workflows/format_check.yaml`)
   - 在 PR 和 push 时自动运行
   - 增量检查（仅检查变更文件）
   - 生成详细报告

2. **自动修复** (`.github/workflows/format_fix.yaml`)
   - 手动触发或通过评论触发
   - 自动创建修复 PR
   - 支持部分工具修复

3. **代码质量** (`.github/workflows/code_quality.yaml`)
   - 静态分析检查
   - 内存泄漏检查
   - 定期运行

### 触发方式

```bash
# 手动触发自动修复
# 在 PR 评论中输入：
/format-fix

# 或通过 GitHub Actions 页面手动触发
```

## 故障排除

### 常见问题

1. **工具未找到**
   ```bash
   # 重新安装工具
   ./scripts/setup_dev_env.sh --tools-only
   
   # 检查工具版本
   clang-format --version
   cmake-format --version
   prettier --version
   ```

2. **格式检查失败**
   ```bash
   # 查看详细错误
   ./scripts/format_check.sh --verbose
   
   # 自动修复
   ./scripts/format_fix.sh --incremental
   ```

3. **pre-commit hooks 不工作**
   ```bash
   # 重新安装 hooks
   pre-commit uninstall
   pre-commit install
   
   # 更新 hooks
   pre-commit autoupdate
   ```

4. **clang-tidy 报错**
   ```bash
   # 确保项目已构建
   mkdir -p build && cd build
   cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   make
   
   # 检查 compile_commands.json 是否存在
   ls build/compile_commands.json
   ```

### 性能优化

1. **增量检查**: 使用 `--incremental` 选项只检查变更文件
2. **并行处理**: 工具会自动使用多核处理
3. **缓存**: CI 中启用了工具缓存，加速构建

### 自定义配置

如需修改格式规则：

1. 编辑对应的配置文件（如 `.clang-format`）
2. 运行格式检查验证配置
3. 提交配置更改

## 最佳实践

### 开发流程

1. **开发前**: 运行 `./check.sh` 确保环境正常
2. **开发中**: 配置 IDE 自动格式化
3. **提交前**: 运行 `./format.sh` 自动修复
4. **提交时**: pre-commit hooks 自动检查
5. **PR 时**: CI 自动验证格式

## 参考资源

- [clang-format 文档](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-tidy 文档](https://clang.llvm.org/extra/clang-tidy/)
- [cmake-format 文档](https://cmake-format.readthedocs.io/)
- [Prettier 文档](https://prettier.io/docs/)
- [EditorConfig 文档](https://editorconfig.org/)
- [pre-commit 文档](https://pre-commit.com/)

---

如有问题或建议，请在项目 Issues 中反馈。
