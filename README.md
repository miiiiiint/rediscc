# RedisCC - Redis C++ Implementation

A high-performance Redis implementation in C++ with modern features and comprehensive tooling.

## Features

- 🚀 **High Performance**: Modern C++20 implementation with optimized data structures
- 📝 **Advanced Logging**: Comprehensive logging system with async support, JSON formatting, and context tracking
- 🔧 **Code Quality**: Automated formatting, static analysis, and quality checks
- 🧪 **Testing**: Comprehensive test suite with memory leak detection
- 📚 **Documentation**: Detailed documentation and examples

## Quick Start

### Prerequisites

- **OS**: Ubuntu 24.10 (minimum Ubuntu 22.04)
- **Compiler**: Clang 18+ or GCC 11+
- **CMake**: 3.5.0+

### Installation

```bash
# Install system dependencies
sudo apt-get install cmake gdb lldb clang clang-tools clang-format automake autogen autoconf build-essential pkg-config

# Clone the repository
git clone <repository-url>
cd rediscc

# Setup development environment (installs formatting tools, pre-commit hooks, etc.)
./scripts/setup_dev_env.sh

# Build the project
mkdir build && cd build
cmake .. -DENABLE_TESTING_BUILD=ON
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

## Development Workflow

### Code Formatting

We use automated code formatting to ensure consistency:

```bash
# Check code format
./check.sh

# Auto-fix formatting issues
./format.sh

# Setup pre-commit hooks (runs automatically on git commit)
pre-commit install
```

For detailed formatting guidelines, see [📖 Format Guide](docs/FORMAT_GUIDE.md) or [🚀 Quick Format Guide](FORMAT_README.md).

### Supported Tools

- **clang-format**: C++ code formatting
- **clang-tidy**: Static analysis and linting
- **cmake-format**: CMake file formatting
- **prettier**: JSON/YAML/Markdown formatting
- **shellcheck**: Shell script analysis

## Project Structure

```
rediscc/
├── include/                 # Header files
│   ├── log.h               # Main logging interface
│   ├── logger_config.h     # Logging configuration
│   ├── logger_manager.h    # Logging manager
│   └── logger_utils.h      # Logging utilities
├── src/                    # Source files
├── tests/                  # Test files
├── examples/               # Example code
├── scripts/                # Development scripts
│   ├── format_check.sh     # Format checking
│   ├── format_fix.sh       # Auto-fix formatting
│   └── setup_dev_env.sh    # Development setup
├── docs/                   # Documentation
├── config/                 # Configuration files
└── .github/workflows/      # CI/CD workflows
```

## Logging System

RedisCC includes a comprehensive logging system with advanced features:

```cpp
#include "log.h"

// Initialize logging
rediscc::logger::InitLogger("config/logger.json");

// Basic logging
REDISCC_INFO("Server started on port {}", 6379);
REDISCC_ERROR("Connection failed: {}", error_msg);

// Context logging
{
    REDISCC_SCOPED_CONTEXT(context);
    REDISCC_INFO("Processing request");
}

// Performance logging
{
    REDISCC_PERF_TIMER("database_query");
    // Your code here
} // Automatically logs execution time
```

Features:
- 🔄 **Async logging** for high performance
- 📊 **JSON formatting** for structured logs
- 🔍 **Context tracking** with request IDs
- 🛡️ **Sensitive data filtering**
- 📈 **Performance monitoring**

See [Logger Documentation](docs/LOGGER_README.md) for details.

## CI/CD

The project includes comprehensive CI/CD workflows:

- **Format Check**: Automatic code formatting verification
- **Auto-Fix**: Automatic formatting issue resolution
- **Code Quality**: Static analysis and memory leak detection
- **Testing**: Comprehensive test suite execution

### GitHub Actions

- 🔍 **Format Check**: Runs on every PR and push
- 🔧 **Auto-Fix**: Triggered by `/format-fix` comment in PRs
- 🧪 **Quality Check**: Static analysis with clang-tidy and cppcheck
- 🔒 **Memory Check**: Valgrind and AddressSanitizer checks

## Contributing

1. **Fork** the repository
2. **Setup** development environment: `./scripts/setup_dev_env.sh`
3. **Create** a feature branch: `git checkout -b feature/amazing-feature`
4. **Make** your changes
5. **Check** formatting: `./check.sh`
6. **Fix** any issues: `./format.sh`
7. **Commit** changes: `git commit -m 'Add amazing feature'`
8. **Push** to branch: `git push origin feature/amazing-feature`
9. **Create** a Pull Request

### Code Style

- Follow the [Format Guide](docs/FORMAT_GUIDE.md)
- Use meaningful variable and function names
- Add comments for complex logic
- Write tests for new features
- Update documentation as needed

## Documentation

- 📖 [Format Guide](docs/FORMAT_GUIDE.md) - Comprehensive formatting guidelines
- 🚀 [Quick Format Guide](FORMAT_README.md) - Quick reference for formatting
- 📝 [Logger Guide](docs/LOGGER_README.md) - Logging system documentation
- 🎯 [Best Practices](docs/LOGGER_BEST_PRACTICES.md) - Logging best practices

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

- 📧 **Issues**: [GitHub Issues](https://github.com/your-repo/rediscc/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/your-repo/rediscc/discussions)
- 📚 **Documentation**: See `docs/` directory

---

**Happy Coding!** 🎉
