#!/bin/bash

# Development Environment Setup Script for RedisCC
# Sets up pre-commit hooks, installs formatting tools, and configures the development environment

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Tool versions
CLANG_FORMAT_VERSION="18"
CMAKE_FORMAT_VERSION="0.6.13"
PRETTIER_VERSION="3.0.0"

# Global variables
INSTALL_TOOLS=true
SETUP_HOOKS=true
CONFIGURE_IDE=true
VERBOSE=false

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

log_debug() {
    if [[ "${VERBOSE}" == "true" ]]; then
        echo -e "${BLUE}[DEBUG]${NC} $*"
    fi
}

# Usage information
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Setup development environment for RedisCC project.

OPTIONS:
    -h, --help              Show this help message
    -v, --verbose           Enable verbose output
    --no-tools              Skip tool installation
    --no-hooks              Skip pre-commit hooks setup
    --no-ide                Skip IDE configuration
    --tools-only            Only install tools
    --hooks-only            Only setup hooks

EXAMPLES:
    $0                      # Full setup
    $0 --no-tools           # Setup without installing tools
    $0 --tools-only         # Only install tools
    $0 -v                   # Verbose setup

EOF
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Detect package manager
detect_package_manager() {
    if command_exists "apt-get"; then
        echo "apt"
    elif command_exists "yum"; then
        echo "yum"
    elif command_exists "dnf"; then
        echo "dnf"
    elif command_exists "brew"; then
        echo "brew"
    elif command_exists "pacman"; then
        echo "pacman"
    else
        echo "unknown"
    fi
}

# Install system packages
install_system_packages() {
    local pm
    pm=$(detect_package_manager)
    
    log_info "Installing system packages using ${pm}..."
    
    case "${pm}" in
        "apt")
            sudo apt-get update
            sudo apt-get install -y \
                clang-format-${CLANG_FORMAT_VERSION} \
                clang-tidy-${CLANG_FORMAT_VERSION} \
                shellcheck \
                python3-pip \
                nodejs \
                npm
            
            # Create symlinks
            sudo ln -sf "/usr/bin/clang-format-${CLANG_FORMAT_VERSION}" /usr/local/bin/clang-format
            sudo ln -sf "/usr/bin/clang-tidy-${CLANG_FORMAT_VERSION}" /usr/local/bin/clang-tidy
            ;;
        "brew")
            brew install clang-format shellcheck python3 node npm
            ;;
        "yum"|"dnf")
            sudo ${pm} install -y clang-tools-extra shellcheck python3-pip nodejs npm
            ;;
        "pacman")
            sudo pacman -S --noconfirm clang extra/clang shellcheck python-pip nodejs npm
            ;;
        *)
            log_warning "Unknown package manager. Please install tools manually:"
            log_warning "  - clang-format"
            log_warning "  - clang-tidy" 
            log_warning "  - shellcheck"
            log_warning "  - python3 with pip"
            log_warning "  - nodejs with npm"
            return 1
            ;;
    esac
    
    log_success "System packages installed"
}

# Install Python packages
install_python_packages() {
    log_info "Installing Python packages..."
    
    if command_exists "pip3"; then
        pip3 install --user "cmake-format==${CMAKE_FORMAT_VERSION}" pre-commit
    elif command_exists "pip"; then
        pip install --user "cmake-format==${CMAKE_FORMAT_VERSION}" pre-commit
    else
        log_error "pip not found. Please install Python pip first."
        return 1
    fi
    
    log_success "Python packages installed"
}

# Install Node.js packages
install_node_packages() {
    log_info "Installing Node.js packages..."
    
    if command_exists "npm"; then
        npm install -g "prettier@${PRETTIER_VERSION}"
    else
        log_error "npm not found. Please install Node.js and npm first."
        return 1
    fi
    
    log_success "Node.js packages installed"
}

# Verify tool installation
verify_tools() {
    log_info "Verifying tool installation..."
    
    local tools=("clang-format" "clang-tidy" "cmake-format" "shellcheck" "prettier" "pre-commit")
    local missing_tools=()
    
    for tool in "${tools[@]}"; do
        if command_exists "${tool}"; then
            log_debug "✓ ${tool} is available"
        else
            missing_tools+=("${tool}")
            log_warning "✗ ${tool} is not available"
        fi
    done
    
    if [[ ${#missing_tools[@]} -eq 0 ]]; then
        log_success "All tools are available"
        return 0
    else
        log_error "Missing tools: ${missing_tools[*]}"
        return 1
    fi
}

# Setup pre-commit hooks
setup_pre_commit_hooks() {
    log_info "Setting up pre-commit hooks..."
    
    # Create .pre-commit-config.yaml if it doesn't exist
    local pre_commit_config="${PROJECT_ROOT}/.pre-commit-config.yaml"
    
    if [[ ! -f "${pre_commit_config}" ]]; then
        cat > "${pre_commit_config}" << 'EOF'
# Pre-commit configuration for RedisCC
# See https://pre-commit.com for more information

repos:
  - repo: local
    hooks:
      - id: format-check
        name: Format Check
        entry: scripts/format_check.sh
        language: script
        args: [--incremental, --verbose]
        pass_filenames: false
        always_run: true
        stages: [commit]

      - id: clang-format
        name: clang-format
        entry: clang-format
        language: system
        files: \.(cpp|cc|cxx|hpp|h|hxx|inc)$
        args: [-i]

      - id: cmake-format
        name: cmake-format
        entry: cmake-format
        language: system
        files: (CMakeLists\.txt|\.cmake)$
        args: [-i]

      - id: prettier
        name: prettier
        entry: prettier
        language: system
        files: \.(json|yaml|yml|md)$
        args: [--write]

      - id: shellcheck
        name: shellcheck
        entry: shellcheck
        language: system
        files: \.sh$
        args: [-e, SC1091]

  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.4.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-json
      - id: check-added-large-files
      - id: check-case-conflict
      - id: check-merge-conflict
      - id: check-symlinks
      - id: check-executables-have-shebangs
      - id: mixed-line-ending
        args: [--fix=lf]

  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v18.1.1
    hooks:
      - id: clang-format
        types_or: [c++, c]
EOF
        log_success "Created .pre-commit-config.yaml"
    fi
    
    # Install pre-commit hooks
    cd "${PROJECT_ROOT}"
    if command_exists "pre-commit"; then
        pre-commit install
        pre-commit install --hook-type commit-msg
        log_success "Pre-commit hooks installed"
    else
        log_error "pre-commit not found. Please install it first."
        return 1
    fi
}

# Create local development scripts
create_dev_scripts() {
    log_info "Creating local development scripts..."
    
    # Create format script
    cat > "${PROJECT_ROOT}/format.sh" << 'EOF'
#!/bin/bash
# Quick format script for local development
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🔧 Formatting code..."

# Run format fix
if [[ -f "${SCRIPT_DIR}/scripts/format_fix.sh" ]]; then
    "${SCRIPT_DIR}/scripts/format_fix.sh" --incremental --verbose "$@"
else
    echo "Error: format_fix.sh not found"
    exit 1
fi

echo "✅ Formatting complete!"
EOF
    
    chmod +x "${PROJECT_ROOT}/format.sh"
    
    # Create check script
    cat > "${PROJECT_ROOT}/check.sh" << 'EOF'
#!/bin/bash
# Quick check script for local development
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🔍 Checking code..."

# Run format check
if [[ -f "${SCRIPT_DIR}/scripts/format_check.sh" ]]; then
    "${SCRIPT_DIR}/scripts/format_check.sh" --incremental --verbose "$@"
else
    echo "Error: format_check.sh not found"
    exit 1
fi

echo "✅ Check complete!"
EOF
    
    chmod +x "${PROJECT_ROOT}/check.sh"
    
    log_success "Development scripts created"
}

# Configure IDE settings
configure_ide() {
    log_info "Configuring IDE settings..."
    
    # Create VSCode settings
    local vscode_dir="${PROJECT_ROOT}/.vscode"
    mkdir -p "${vscode_dir}"
    
    # VSCode settings.json
    cat > "${vscode_dir}/settings.json" << 'EOF'
{
  "editor.formatOnSave": true,
  "editor.codeActionsOnSave": {
    "source.fixAll": true
  },
  "files.trimTrailingWhitespace": true,
  "files.insertFinalNewline": true,
  "files.trimFinalNewlines": true,
  "C_Cpp.clang_format_style": "file",
  "C_Cpp.clang_format_fallbackStyle": "LLVM",
  "cmake.format.allowOptionalArgumentIndentation": true,
  "cmake.format.keepEmptyLines": true,
  "prettier.configPath": ".prettierrc",
  "shellcheck.enable": true,
  "shellcheck.run": "onSave"
}
EOF
    
    # VSCode extensions.json
    cat > "${vscode_dir}/extensions.json" << 'EOF'
{
  "recommendations": [
    "ms-vscode.cpptools",
    "ms-vscode.cmake-tools",
    "twxs.cmake",
    "esbenp.prettier-vscode",
    "timonwong.shellcheck",
    "ms-vscode.vscode-clangd",
    "llvm-vs-code-extensions.vscode-clangd"
  ]
}
EOF
    
    log_success "IDE configuration created"
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                usage
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            --no-tools)
                INSTALL_TOOLS=false
                shift
                ;;
            --no-hooks)
                SETUP_HOOKS=false
                shift
                ;;
            --no-ide)
                CONFIGURE_IDE=false
                shift
                ;;
            --tools-only)
                INSTALL_TOOLS=true
                SETUP_HOOKS=false
                CONFIGURE_IDE=false
                shift
                ;;
            --hooks-only)
                INSTALL_TOOLS=false
                SETUP_HOOKS=true
                CONFIGURE_IDE=false
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done
}

# Main function
main() {
    log_info "Setting up RedisCC development environment..."
    
    # Parse arguments
    parse_arguments "$@"
    
    # Show configuration
    if [[ "${VERBOSE}" == "true" ]]; then
        log_debug "Configuration:"
        log_debug "  Install tools: ${INSTALL_TOOLS}"
        log_debug "  Setup hooks: ${SETUP_HOOKS}"
        log_debug "  Configure IDE: ${CONFIGURE_IDE}"
        log_debug "  Project root: ${PROJECT_ROOT}"
    fi
    
    # Install tools
    if [[ "${INSTALL_TOOLS}" == "true" ]]; then
        install_system_packages
        install_python_packages
        install_node_packages
        verify_tools
    fi
    
    # Setup pre-commit hooks
    if [[ "${SETUP_HOOKS}" == "true" ]]; then
        setup_pre_commit_hooks
    fi
    
    # Configure IDE
    if [[ "${CONFIGURE_IDE}" == "true" ]]; then
        configure_ide
    fi
    
    # Create development scripts
    create_dev_scripts
    
    log_success "Development environment setup completed!"
    
    # Show next steps
    echo ""
    log_info "Next steps:"
    echo "  1. Run './check.sh' to check code formatting"
    echo "  2. Run './format.sh' to auto-fix formatting issues"
    echo "  3. Use 'git commit' to trigger pre-commit hooks"
    echo "  4. Configure your IDE with the provided settings"
    echo ""
    log_info "For more information, see docs/FORMAT_GUIDE.md"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
