#!/bin/bash

# ============================================================
#  SynGT C++ - Build Script for Linux/Mac/MSYS2
# ============================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Default settings
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
BUILD_TYPE="Release"
CLEAN_BUILD=0

# Functions
print_header() {
    echo ""
    echo "============================================================"
    echo "  $1"
    echo "============================================================"
    echo ""
}

print_success() {
    echo -e "${GREEN}[√]${NC} $1"
}

print_error() {
    echo -e "${RED}[X]${NC} $1"
}

print_info() {
    echo -e "${CYAN}[*]${NC} $1"
}

show_help() {
    cat << EOF

Usage: ./build.sh [options]

Options:
  clean          Clean build directory before building
  debug          Build in Debug mode
  release        Build in Release mode (default)
  help           Show this help

Examples:
  ./build.sh                    # Build in Release mode
  ./build.sh clean              # Clean and build
  ./build.sh debug              # Build in Debug mode
  ./build.sh clean release      # Clean and build Release

EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        clean)
            CLEAN_BUILD=1
            shift
            ;;
        debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        release)
            BUILD_TYPE="Release"
            shift
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            ;;
    esac
done

print_header "SynGT C++ Build Script"

# Clean if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    print_info "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    print_success "Build directory cleaned"
fi

# Create build directory
if [ ! -d "${BUILD_DIR}" ]; then
    print_info "Creating build directory..."
    mkdir -p "${BUILD_DIR}"
fi

# Enter build directory
cd "${BUILD_DIR}"

# Configure CMake
print_info "Configuring CMake (${BUILD_TYPE})..."
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_CLI=ON \
    -DBUILD_GUI=OFF \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=OFF \
    ..

print_success "CMake configured successfully"

# Build
echo ""
print_info "Building project..."
ninja

# Success message
echo ""
print_header "Build completed successfully!"

echo "Executable location:"
echo "  ${BUILD_DIR}/bin/syngt_cli"
echo ""
echo "To run:"
echo "  cd build/bin"
echo "  ./syngt_cli --help"
echo ""

exit 0