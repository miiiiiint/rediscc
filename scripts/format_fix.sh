#!/bin/bash

# Auto-fix Format Issues Script for RedisCC
# Automatically fixes formatting issues found by format_check.sh

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

# Global variables
BACKUP_DIR=""
CREATE_BACKUP=true
INCREMENTAL_MODE=false
TARGET_BRANCH="main"
VERBOSE=false
DRY_RUN=false
TOOLS_TO_RUN=()

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

Automatically fix formatting issues in the codebase.

OPTIONS:
    -h, --help              Show this help message
    -i, --incremental       Fix only changed files (incremental mode)
    -v, --verbose           Enable verbose output
    -n, --dry-run           Show what would be fixed without making changes
    -b, --branch BRANCH     Target branch for incremental mode (default: main)
    --no-backup             Don't create backup before fixing
    --backup-dir DIR        Custom backup directory
    -t, --tools TOOLS       Comma-separated list of tools to run (default: all)

EXAMPLES:
    $0                      # Fix all files
    $0 -i                   # Fix only changed files
    $0 -n                   # Dry-run mode (show what would be fixed)
    $0 -t clang-format      # Fix only with clang-format
    $0 --no-backup          # Fix without creating backup

EOF
}

# Create backup of files
create_backup() {
    if [[ "${CREATE_BACKUP}" == "false" ]]; then
        return 0
    fi
    
    if [[ -z "${BACKUP_DIR}" ]]; then
        BACKUP_DIR="${PROJECT_ROOT}/.format_backup_$(date +%Y%m%d_%H%M%S)"
    fi
    
    log_info "Creating backup in: ${BACKUP_DIR}"
    mkdir -p "${BACKUP_DIR}"
    
    # Copy all source files to backup
    find "${PROJECT_ROOT}" -type f \( \
        -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o \
        -name "*.hpp" -o -name "*.h" -o -name "*.hxx" -o -name "*.inc" -o \
        -name "CMakeLists.txt" -o -name "*.cmake" -o \
        -name "*.sh" -o -name "*.json" -o -name "*.yaml" -o -name "*.yml" -o -name "*.md" \
    \) -not -path "*/build/*" -not -path "*/deps/*" -not -path "*/.git/*" \
    -exec cp --parents {} "${BACKUP_DIR}/" \;
    
    log_success "Backup created successfully"
}

# Restore from backup
restore_backup() {
    if [[ -z "${BACKUP_DIR}" || ! -d "${BACKUP_DIR}" ]]; then
        log_error "No backup directory found"
        return 1
    fi
    
    log_info "Restoring from backup: ${BACKUP_DIR}"
    
    # Copy files back from backup
    find "${BACKUP_DIR}" -type f -exec bash -c '
        src="$1"
        dst="${src#'"${BACKUP_DIR}"'/}"
        dst="'"${PROJECT_ROOT}"'/${dst}"
        mkdir -p "$(dirname "${dst}")"
        cp "${src}" "${dst}"
    ' _ {} \;
    
    log_success "Restored from backup successfully"
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
            --no-backup)
                CREATE_BACKUP=false
                shift
                ;;
            --backup-dir)
                BACKUP_DIR="$2"
                shift 2
                ;;
            -t|--tools)
                IFS=',' read -ra TOOLS_TO_RUN <<< "$2"
                shift 2
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done
}

# Run format fix
run_format_fix() {
    local format_check_script="${SCRIPT_DIR}/format_check.sh"
    
    if [[ ! -f "${format_check_script}" ]]; then
        log_error "Format check script not found: ${format_check_script}"
        return 1
    fi
    
    # Build arguments for format_check.sh
    local args=("--fix")
    
    if [[ "${INCREMENTAL_MODE}" == "true" ]]; then
        args+=("--incremental" "--branch" "${TARGET_BRANCH}")
    fi
    
    if [[ "${VERBOSE}" == "true" ]]; then
        args+=("--verbose")
    fi
    
    if [[ "${DRY_RUN}" == "true" ]]; then
        args+=("--dry-run")
    fi
    
    if [[ ${#TOOLS_TO_RUN[@]} -gt 0 ]]; then
        local tools_str
        IFS=',' eval 'tools_str="${TOOLS_TO_RUN[*]}"'
        args+=("--tools" "${tools_str}")
    fi
    
    log_info "Running format check with auto-fix..."
    log_debug "Command: ${format_check_script} ${args[*]}"
    
    # Run the format check with fix mode
    if "${format_check_script}" "${args[@]}"; then
        log_success "Format fix completed successfully"
        return 0
    else
        log_error "Format fix encountered issues"
        return 1
    fi
}

# Cleanup function
cleanup() {
    if [[ -n "${BACKUP_DIR}" && -d "${BACKUP_DIR}" && "${CREATE_BACKUP}" == "true" ]]; then
        read -p "Keep backup directory ${BACKUP_DIR}? [y/N] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "${BACKUP_DIR}"
            log_info "Backup directory removed"
        else
            log_info "Backup directory kept: ${BACKUP_DIR}"
        fi
    fi
}

# Setup trap for cleanup
trap cleanup EXIT

# Main function
main() {
    log_info "Starting auto-fix format script..."
    
    # Parse arguments
    parse_arguments "$@"
    
    # Show configuration
    if [[ "${VERBOSE}" == "true" ]]; then
        log_debug "Configuration:"
        log_debug "  Incremental mode: ${INCREMENTAL_MODE}"
        log_debug "  Dry-run mode: ${DRY_RUN}"
        log_debug "  Create backup: ${CREATE_BACKUP}"
        log_debug "  Target branch: ${TARGET_BRANCH}"
        log_debug "  Project root: ${PROJECT_ROOT}"
        if [[ ${#TOOLS_TO_RUN[@]} -gt 0 ]]; then
            log_debug "  Tools to run: ${TOOLS_TO_RUN[*]}"
        fi
    fi
    
    # Create backup if not in dry-run mode
    if [[ "${DRY_RUN}" == "false" ]]; then
        create_backup
    fi
    
    # Run format fix
    if run_format_fix; then
        log_success "All formatting issues have been fixed!"
        
        # Show git status if there are changes
        if [[ "${DRY_RUN}" == "false" ]] && git diff --quiet; then
            log_info "No changes were made to the files"
        elif [[ "${DRY_RUN}" == "false" ]]; then
            log_info "Files have been modified. Git status:"
            git status --porcelain
            echo ""
            log_info "To commit these changes, run:"
            echo "  git add ."
            echo "  git commit -m 'fix: auto-format code'"
        fi
        
        exit 0
    else
        log_error "Format fix failed!"
        
        # Offer to restore backup
        if [[ "${DRY_RUN}" == "false" && "${CREATE_BACKUP}" == "true" ]]; then
            read -p "Restore from backup? [y/N] " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                restore_backup
            fi
        fi
        
        exit 1
    fi
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
