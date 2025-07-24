#!/bin/bash

# Enhanced Format Check Script for RedisCC
# Supports incremental checking, multiple tools, and better error reporting

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TOOLS_DIR="${SCRIPT_DIR}/tools"

# Default configuration
DEFAULT_TARGET_BRANCH="main"
DEFAULT_CLANG_FORMAT_VERSION="18"
DEFAULT_PARALLEL_JOBS="$(nproc 2>/dev/null || echo 4)"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Global variables
INCREMENTAL_MODE=false
AUTO_FIX=false
VERBOSE=false
DRY_RUN=false
TARGET_BRANCH="${DEFAULT_TARGET_BRANCH}"
PARALLEL_JOBS="${DEFAULT_PARALLEL_JOBS}"
TOOLS_TO_RUN=()
FILES_TO_CHECK=()
TEMP_DIR=""
TOTAL_FILES=0
TOTAL_ISSUES=0
TOTAL_FIXED=0

# Tool configurations
declare -A TOOL_CONFIGS=(
    ["clang-format"]="enabled"
    ["clang-tidy"]="enabled"
    ["cmake-format"]="enabled"
    ["shellcheck"]="enabled"
    ["prettier"]="enabled"
)

# File patterns for different tools
declare -A FILE_PATTERNS=(
    ["clang-format"]="*.cpp *.cc *.cxx *.hpp *.h *.hxx *.inc"
    ["clang-tidy"]="*.cpp *.cc *.cxx"
    ["cmake-format"]="CMakeLists.txt *.cmake"
    ["shellcheck"]="*.sh"
    ["prettier"]="*.json *.yaml *.yml *.md"
)

# Usage information
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Enhanced format checking script with support for multiple tools and incremental checking.

OPTIONS:
    -h, --help              Show this help message
    -i, --incremental       Enable incremental mode (check only changed files)
    -f, --fix               Enable auto-fix mode
    -v, --verbose           Enable verbose output
    -n, --dry-run           Show what would be done without making changes
    -b, --branch BRANCH     Target branch for incremental mode (default: ${DEFAULT_TARGET_BRANCH})
    -j, --jobs JOBS         Number of parallel jobs (default: ${DEFAULT_PARALLEL_JOBS})
    -t, --tools TOOLS       Comma-separated list of tools to run (default: all enabled)
    --files FILES           Comma-separated list of specific files to check
    --disable-tool TOOL     Disable a specific tool
    --enable-tool TOOL      Enable a specific tool
    --list-tools            List available tools and their status

TOOLS:
    clang-format           C++ code formatting
    clang-tidy             C++ static analysis
    cmake-format           CMake file formatting
    shellcheck             Shell script analysis
    prettier               JSON/YAML/Markdown formatting

EXAMPLES:
    $0                                    # Check all files with all tools
    $0 -i                                # Check only changed files
    $0 -i -f                             # Check and fix changed files
    $0 -t clang-format,clang-tidy        # Run only specific tools
    $0 --files "src/main.cpp,src/util.h" # Check specific files
    $0 -v -n                             # Verbose dry-run mode

EOF
}

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
        echo -e "${PURPLE}[DEBUG]${NC} $*"
    fi
}

# Cleanup function
cleanup() {
    if [[ -n "${TEMP_DIR}" && -d "${TEMP_DIR}" ]]; then
        rm -rf "${TEMP_DIR}"
    fi
}

# Setup trap for cleanup
trap cleanup EXIT

# Initialize temporary directory
init_temp_dir() {
    TEMP_DIR=$(mktemp -d -t format_check.XXXXXX)
    log_debug "Created temporary directory: ${TEMP_DIR}"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install or check tool availability
check_tool_availability() {
    local tool="$1"
    
    case "${tool}" in
        "clang-format")
            if ! command_exists "clang-format"; then
                log_warning "clang-format not found, attempting to install..."
                if command_exists "apt-get"; then
                    sudo apt-get update && sudo apt-get install -y "clang-format-${DEFAULT_CLANG_FORMAT_VERSION}"
                elif command_exists "brew"; then
                    brew install clang-format
                else
                    log_error "Cannot install clang-format automatically"
                    return 1
                fi
            fi
            ;;
        "clang-tidy")
            if ! command_exists "clang-tidy"; then
                log_warning "clang-tidy not found, attempting to install..."
                if command_exists "apt-get"; then
                    sudo apt-get update && sudo apt-get install -y "clang-tidy-${DEFAULT_CLANG_FORMAT_VERSION}"
                elif command_exists "brew"; then
                    brew install llvm
                else
                    log_error "Cannot install clang-tidy automatically"
                    return 1
                fi
            fi
            ;;
        "cmake-format")
            if ! command_exists "cmake-format"; then
                log_warning "cmake-format not found, attempting to install..."
                if command_exists "pip3"; then
                    pip3 install cmake-format
                elif command_exists "pip"; then
                    pip install cmake-format
                else
                    log_error "Cannot install cmake-format automatically"
                    return 1
                fi
            fi
            ;;
        "shellcheck")
            if ! command_exists "shellcheck"; then
                log_warning "shellcheck not found, attempting to install..."
                if command_exists "apt-get"; then
                    sudo apt-get update && sudo apt-get install -y shellcheck
                elif command_exists "brew"; then
                    brew install shellcheck
                else
                    log_error "Cannot install shellcheck automatically"
                    return 1
                fi
            fi
            ;;
        "prettier")
            if ! command_exists "prettier"; then
                log_warning "prettier not found, attempting to install..."
                if command_exists "npm"; then
                    npm install -g prettier
                elif command_exists "yarn"; then
                    yarn global add prettier
                else
                    log_error "Cannot install prettier automatically"
                    return 1
                fi
            fi
            ;;
        *)
            log_error "Unknown tool: ${tool}"
            return 1
            ;;
    esac
    
    log_debug "Tool ${tool} is available"
    return 0
}

# Get list of files to check
get_files_to_check() {
    local files=()
    
    if [[ ${#FILES_TO_CHECK[@]} -gt 0 ]]; then
        # Use explicitly specified files
        files=("${FILES_TO_CHECK[@]}")
        log_debug "Using explicitly specified files: ${files[*]}"
    elif [[ "${INCREMENTAL_MODE}" == "true" ]]; then
        # Get changed files from git
        log_info "Getting changed files from git (comparing with ${TARGET_BRANCH})..."
        
        # Ensure we have the target branch
        if ! git show-ref --verify --quiet "refs/heads/${TARGET_BRANCH}" && \
           ! git show-ref --verify --quiet "refs/remotes/origin/${TARGET_BRANCH}"; then
            log_warning "Target branch ${TARGET_BRANCH} not found, using HEAD~1"
            TARGET_BRANCH="HEAD~1"
        fi
        
        # Get list of changed files
        mapfile -t files < <(git diff --name-only "${TARGET_BRANCH}...HEAD" 2>/dev/null || \
                            git diff --name-only HEAD~1 2>/dev/null || \
                            find . -name "*.cpp" -o -name "*.h" -o -name "*.cc" | head -10)
        
        log_debug "Found ${#files[@]} changed files"
    else
        # Get all relevant files
        log_info "Getting all relevant files..."
        mapfile -t files < <(find "${PROJECT_ROOT}" -type f \( \
            -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o \
            -name "*.hpp" -o -name "*.h" -o -name "*.hxx" -o -name "*.inc" -o \
            -name "CMakeLists.txt" -o -name "*.cmake" -o \
            -name "*.sh" -o -name "*.json" -o -name "*.yaml" -o -name "*.yml" -o -name "*.md" \
        \) -not -path "*/build/*" -not -path "*/deps/*" -not -path "*/.git/*")
        
        log_debug "Found ${#files[@]} total files"
    fi
    
    # Filter out non-existent files and normalize paths
    local filtered_files=()
    for file in "${files[@]}"; do
        if [[ -f "${file}" ]]; then
            # Convert to absolute path relative to project root
            if [[ "${file}" = /* ]]; then
                filtered_files+=("${file}")
            else
                filtered_files+=("${PROJECT_ROOT}/${file}")
            fi
        fi
    done
    
    files=("${filtered_files[@]}")
    TOTAL_FILES=${#files[@]}
    
    if [[ ${TOTAL_FILES} -eq 0 ]]; then
        log_warning "No files found to check"
        return 1
    fi
    
    log_info "Will check ${TOTAL_FILES} files"
    printf '%s\n' "${files[@]}"
}

# Filter files by pattern for a specific tool
filter_files_for_tool() {
    local tool="$1"
    shift
    local all_files=("$@")
    local filtered_files=()
    local patterns="${FILE_PATTERNS[$tool]}"

    for file in "${all_files[@]}"; do
        local basename_file
        basename_file=$(basename "${file}")
        local dirname_file
        dirname_file=$(dirname "${file}")

        for pattern in ${patterns}; do
            if [[ "${basename_file}" == ${pattern} ]] || [[ "${file}" == */${pattern} ]]; then
                filtered_files+=("${file}")
                break
            fi
        done
    done

    printf '%s\n' "${filtered_files[@]}"
}

# Run clang-format check
run_clang_format_check() {
    local files=("$@")
    local issues_found=false
    local config_file="${PROJECT_ROOT}/.clang-format"

    if [[ ! -f "${config_file}" ]]; then
        log_warning "No .clang-format config found, using default style"
        config_file=""
    fi

    log_debug "Checking ${#files[@]} files with clang-format"

    for file in "${files[@]}"; do
        local temp_file="${TEMP_DIR}/$(basename "${file}").formatted"

        # Format the file
        if [[ -n "${config_file}" ]]; then
            clang-format --style=file:"${config_file}" "${file}" > "${temp_file}"
        else
            clang-format "${file}" > "${temp_file}"
        fi

        # Check for differences
        if ! diff -q "${file}" "${temp_file}" >/dev/null 2>&1; then
            issues_found=true
            ((TOTAL_ISSUES++))

            log_warning "Formatting issues in: ${file}"

            if [[ "${VERBOSE}" == "true" ]]; then
                echo "--- Diff for ${file} ---"
                diff -u "${file}" "${temp_file}" || true
                echo "--- End diff ---"
            fi

            # Auto-fix if requested
            if [[ "${AUTO_FIX}" == "true" && "${DRY_RUN}" == "false" ]]; then
                cp "${temp_file}" "${file}"
                log_info "Fixed formatting in: ${file}"
                ((TOTAL_FIXED++))
            fi
        fi

        rm -f "${temp_file}"
    done

    if [[ "${issues_found}" == "true" ]]; then
        return 1
    fi
    return 0
}

# Run clang-tidy check
run_clang_tidy_check() {
    local files=("$@")
    local issues_found=false
    local config_file="${PROJECT_ROOT}/.clang-tidy"

    if [[ ! -f "${config_file}" ]]; then
        log_debug "No .clang-tidy config found, using default checks"
    fi

    log_debug "Checking ${#files[@]} files with clang-tidy"

    # Check if compile_commands.json exists
    local compile_commands="${PROJECT_ROOT}/build/compile_commands.json"
    local clang_tidy_args=()

    if [[ -f "${compile_commands}" ]]; then
        clang_tidy_args+=("-p" "$(dirname "${compile_commands}")")
        log_debug "Using compile commands from: ${compile_commands}"
    else
        log_warning "No compile_commands.json found, clang-tidy may have limited functionality"
        clang_tidy_args+=("--" "-I${PROJECT_ROOT}/include" "-std=c++20")
    fi

    for file in "${files[@]}"; do
        local output_file="${TEMP_DIR}/$(basename "${file}").tidy"

        # Run clang-tidy
        if clang-tidy "${file}" "${clang_tidy_args[@]}" > "${output_file}" 2>&1; then
            # Check if there are any warnings/errors
            if [[ -s "${output_file}" ]] && grep -q "warning:\|error:" "${output_file}"; then
                issues_found=true
                ((TOTAL_ISSUES++))

                log_warning "Static analysis issues in: ${file}"

                if [[ "${VERBOSE}" == "true" ]]; then
                    echo "--- clang-tidy output for ${file} ---"
                    cat "${output_file}"
                    echo "--- End output ---"
                fi
            fi
        else
            log_error "clang-tidy failed for: ${file}"
            issues_found=true
            ((TOTAL_ISSUES++))
        fi

        rm -f "${output_file}"
    done

    if [[ "${issues_found}" == "true" ]]; then
        return 1
    fi
    return 0
}

# Run cmake-format check
run_cmake_format_check() {
    local files=("$@")
    local issues_found=false
    local config_file="${PROJECT_ROOT}/.cmake-format.yaml"

    log_debug "Checking ${#files[@]} files with cmake-format"

    for file in "${files[@]}"; do
        local temp_file="${TEMP_DIR}/$(basename "${file}").formatted"

        # Format the file
        local cmake_format_args=()
        if [[ -f "${config_file}" ]]; then
            cmake_format_args+=("--config-file" "${config_file}")
        fi

        if cmake-format "${cmake_format_args[@]}" "${file}" > "${temp_file}" 2>/dev/null; then
            # Check for differences
            if ! diff -q "${file}" "${temp_file}" >/dev/null 2>&1; then
                issues_found=true
                ((TOTAL_ISSUES++))

                log_warning "CMake formatting issues in: ${file}"

                if [[ "${VERBOSE}" == "true" ]]; then
                    echo "--- Diff for ${file} ---"
                    diff -u "${file}" "${temp_file}" || true
                    echo "--- End diff ---"
                fi

                # Auto-fix if requested
                if [[ "${AUTO_FIX}" == "true" && "${DRY_RUN}" == "false" ]]; then
                    cp "${temp_file}" "${file}"
                    log_info "Fixed CMake formatting in: ${file}"
                    ((TOTAL_FIXED++))
                fi
            fi
        else
            log_error "cmake-format failed for: ${file}"
            issues_found=true
            ((TOTAL_ISSUES++))
        fi

        rm -f "${temp_file}"
    done

    if [[ "${issues_found}" == "true" ]]; then
        return 1
    fi
    return 0
}

# Run shellcheck
run_shellcheck_check() {
    local files=("$@")
    local issues_found=false

    log_debug "Checking ${#files[@]} files with shellcheck"

    for file in "${files[@]}"; do
        local output_file="${TEMP_DIR}/$(basename "${file}").shellcheck"

        # Run shellcheck
        if shellcheck -f gcc "${file}" > "${output_file}" 2>&1; then
            # No issues found
            :
        else
            # Check if there are actual issues (not just missing shebang warnings)
            if [[ -s "${output_file}" ]]; then
                issues_found=true
                ((TOTAL_ISSUES++))

                log_warning "Shell script issues in: ${file}"

                if [[ "${VERBOSE}" == "true" ]]; then
                    echo "--- shellcheck output for ${file} ---"
                    cat "${output_file}"
                    echo "--- End output ---"
                fi
            fi
        fi

        rm -f "${output_file}"
    done

    if [[ "${issues_found}" == "true" ]]; then
        return 1
    fi
    return 0
}

# Run prettier check
run_prettier_check() {
    local files=("$@")
    local issues_found=false
    local config_file="${PROJECT_ROOT}/.prettierrc"

    log_debug "Checking ${#files[@]} files with prettier"

    for file in "${files[@]}"; do
        local temp_file="${TEMP_DIR}/$(basename "${file}").formatted"

        # Format the file
        local prettier_args=("--write")
        if [[ -f "${config_file}" ]]; then
            prettier_args+=("--config" "${config_file}")
        fi

        # Copy file to temp location and format it
        cp "${file}" "${temp_file}"

        if prettier "${prettier_args[@]}" "${temp_file}" >/dev/null 2>&1; then
            # Check for differences
            if ! diff -q "${file}" "${temp_file}" >/dev/null 2>&1; then
                issues_found=true
                ((TOTAL_ISSUES++))

                log_warning "Prettier formatting issues in: ${file}"

                if [[ "${VERBOSE}" == "true" ]]; then
                    echo "--- Diff for ${file} ---"
                    diff -u "${file}" "${temp_file}" || true
                    echo "--- End diff ---"
                fi

                # Auto-fix if requested
                if [[ "${AUTO_FIX}" == "true" && "${DRY_RUN}" == "false" ]]; then
                    cp "${temp_file}" "${file}"
                    log_info "Fixed prettier formatting in: ${file}"
                    ((TOTAL_FIXED++))
                fi
            fi
        else
            log_error "prettier failed for: ${file}"
            issues_found=true
            ((TOTAL_ISSUES++))
        fi

        rm -f "${temp_file}"
    done

    if [[ "${issues_found}" == "true" ]]; then
        return 1
    fi
    return 0
}

# Run tool check
run_tool_check() {
    local tool="$1"
    shift
    local all_files=("$@")

    # Filter files for this tool
    local files
    mapfile -t files < <(filter_files_for_tool "${tool}" "${all_files[@]}")

    if [[ ${#files[@]} -eq 0 ]]; then
        log_debug "No files to check for tool: ${tool}"
        return 0
    fi

    log_info "Running ${tool} on ${#files[@]} files..."

    case "${tool}" in
        "clang-format")
            run_clang_format_check "${files[@]}"
            ;;
        "clang-tidy")
            run_clang_tidy_check "${files[@]}"
            ;;
        "cmake-format")
            run_cmake_format_check "${files[@]}"
            ;;
        "shellcheck")
            run_shellcheck_check "${files[@]}"
            ;;
        "prettier")
            run_prettier_check "${files[@]}"
            ;;
        *)
            log_error "Unknown tool: ${tool}"
            return 1
            ;;
    esac
}

# Generate summary report
generate_summary_report() {
    local report_file="${TEMP_DIR}/format_check_report.txt"

    {
        echo "Format Check Summary Report"
        echo "=========================="
        echo "Generated: $(date)"
        echo "Project: $(basename "${PROJECT_ROOT}")"
        echo "Mode: $(if [[ "${INCREMENTAL_MODE}" == "true" ]]; then echo "Incremental"; else echo "Full"; fi)"
        echo "Auto-fix: $(if [[ "${AUTO_FIX}" == "true" ]]; then echo "Enabled"; else echo "Disabled"; fi)"
        echo ""
        echo "Statistics:"
        echo "  Total files checked: ${TOTAL_FILES}"
        echo "  Total issues found: ${TOTAL_ISSUES}"
        echo "  Total issues fixed: ${TOTAL_FIXED}"
        echo ""

        if [[ "${TOTAL_ISSUES}" -gt 0 ]]; then
            echo "Issues found! Please review the output above for details."
            if [[ "${AUTO_FIX}" == "false" ]]; then
                echo "To automatically fix issues, run with --fix option."
            fi
        else
            echo "No formatting issues found. Great job!"
        fi
    } > "${report_file}"

    if [[ "${VERBOSE}" == "true" ]]; then
        echo ""
        echo "=== SUMMARY REPORT ==="
        cat "${report_file}"
        echo "======================"
    fi

    # Save report to project root if requested
    if [[ -n "${GITHUB_ACTIONS:-}" ]]; then
        cp "${report_file}" "${PROJECT_ROOT}/format_check_report.txt"
        echo "::notice::Format check report saved to format_check_report.txt"
    fi
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                usage
                exit 0
                ;;
            -i|--incremental)
                INCREMENTAL_MODE=true
                shift
                ;;
            -f|--fix)
                AUTO_FIX=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -n|--dry-run)
                DRY_RUN=true
                shift
                ;;
            -b|--branch)
                TARGET_BRANCH="$2"
                shift 2
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            -t|--tools)
                IFS=',' read -ra TOOLS_TO_RUN <<< "$2"
                shift 2
                ;;
            --files)
                IFS=',' read -ra FILES_TO_CHECK <<< "$2"
                shift 2
                ;;
            --disable-tool)
                TOOL_CONFIGS["$2"]="disabled"
                shift 2
                ;;
            --enable-tool)
                TOOL_CONFIGS["$2"]="enabled"
                shift 2
                ;;
            --list-tools)
                echo "Available tools:"
                for tool in "${!TOOL_CONFIGS[@]}"; do
                    echo "  ${tool}: ${TOOL_CONFIGS[$tool]}"
                done
                exit 0
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
    log_info "Starting enhanced format check..."
    
    # Initialize
    init_temp_dir
    
    # Parse arguments
    parse_arguments "$@"
    
    # Show configuration
    if [[ "${VERBOSE}" == "true" ]]; then
        log_debug "Configuration:"
        log_debug "  Incremental mode: ${INCREMENTAL_MODE}"
        log_debug "  Auto-fix mode: ${AUTO_FIX}"
        log_debug "  Dry-run mode: ${DRY_RUN}"
        log_debug "  Target branch: ${TARGET_BRANCH}"
        log_debug "  Parallel jobs: ${PARALLEL_JOBS}"
        log_debug "  Project root: ${PROJECT_ROOT}"
    fi
    
    # Get files to check
    local files
    if ! mapfile -t files < <(get_files_to_check); then
        exit 0
    fi

    # Determine which tools to run
    local tools_to_run=()
    if [[ ${#TOOLS_TO_RUN[@]} -gt 0 ]]; then
        tools_to_run=("${TOOLS_TO_RUN[@]}")
    else
        for tool in "${!TOOL_CONFIGS[@]}"; do
            if [[ "${TOOL_CONFIGS[$tool]}" == "enabled" ]]; then
                tools_to_run+=("${tool}")
            fi
        done
    fi

    log_info "Will run tools: ${tools_to_run[*]}"

    # Check tool availability
    for tool in "${tools_to_run[@]}"; do
        if ! check_tool_availability "${tool}"; then
            log_error "Tool ${tool} is not available and cannot be installed"
            exit 1
        fi
    done

    # Run format checks
    local overall_success=true
    for tool in "${tools_to_run[@]}"; do
        log_info "Running ${tool}..."
        if ! run_tool_check "${tool}" "${files[@]}"; then
            overall_success=false
        fi
    done

    # Generate summary report
    generate_summary_report

    if [[ "${overall_success}" == "true" ]]; then
        log_success "Format check completed successfully!"
        log_info "Summary: ${TOTAL_FILES} files checked, ${TOTAL_ISSUES} issues found, ${TOTAL_FIXED} issues fixed"
        exit 0
    else
        log_error "Format check failed!"
        log_info "Summary: ${TOTAL_FILES} files checked, ${TOTAL_ISSUES} issues found, ${TOTAL_FIXED} issues fixed"
        exit 1
    fi
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
