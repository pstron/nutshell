#!/bin/bash

# Required Packages:
# arm-none-eabi-gcc
# arm-none-eabi-newlib
# arm-none-eabi-binutils
# cmake ninja
# stlink openocd
# gdb-arm-none-eabi

# build.sh
# A simple bash script for building and testing in pacman style
# Author: NUT-Shell Xiao Xiyu
# Date: Oct 2025

set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

TOOLCHAIN_FILE="$ROOT_DIR/cmake/gcc-arm-none-eabi.cmake"

# Customize these variables for your project
PROJECT_NAME="nutshell"
INTERFACE_CFG="interface/stlink.cfg"
TARGET_CFG="target/stm32f1x.cfg"

# Color definitions (pacman style)
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Pacman-style output functions
print_action() {
    echo -e "${GREEN}::${NC} ${BOLD}$1${NC}"
}

print_progress() {
    echo -e "${BLUE}==>${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}==> WARNING:${NC} $1"
}

print_error() {
    echo -e "${RED}==> ERROR:${NC} $1"
}

print_success() {
    echo -e "${GREEN}==> SUCCESS:${NC} $1"
}

print_info() {
    echo -e "${CYAN}==> INFO:${NC} $1"
}

# Function to build project
build_project() {
    local build_type=$1
    print_action "Building $build_type version..."

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    print_progress "Running CMake..."
    cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" -DCMAKE_BUILD_TYPE="$build_type" ..

    print_progress "Running Ninja..."
    ninja

    print_success "Build complete!"
}

# Function to flash the binary
flash_binary() {
    print_action "Flashing binary to device..."

    if [[ ! -f "${BUILD_DIR}/${PROJECT_NAME}.elf" ]]; then
        print_error "Binary not found. Please build first."
        exit 1
    fi

    print_progress "Starting OpenOCD..."
    openocd -f "$INTERFACE_CFG" -f "$TARGET_CFG" \
        -c "program ${BUILD_DIR}/${PROJECT_NAME}.elf verify reset exit"

    print_success "Flash complete!"
}

# Function to debug
debug_project() {
    print_action "Starting debug session..."

    if [[ ! -f "${BUILD_DIR}/${PROJECT_NAME}.elf" ]]; then
        print_error "Debug binary not found. Please build debug version first."
        exit 1
    fi

    print_progress "Starting OpenOCD daemon..."
    openocd -f "$INTERFACE_CFG" -f "$TARGET_CFG" &
    OCD_PID=$!

    sleep 1

    print_progress "Starting GDB session..."
    arm-none-eabi-gdb "${BUILD_DIR}/${PROJECT_NAME}.elf" \
        -ex "target remote :3333" \
        -ex "monitor reset halt" \
        -ex "layout src"

    # Clean up
    print_progress "Cleaning up..."
    kill $OCD_PID 2>/dev/null || true
    print_success "Debug session ended."
}

# Function to show build information
info_project() {
    print_action "Project Information"
    if [[ -f "${BUILD_DIR}/${PROJECT_NAME}.elf" ]]; then
        print_progress "Binary size information:"
        arm-none-eabi-size "${BUILD_DIR}/${PROJECT_NAME}.elf" | tail -n +2

        local build_type=$(grep CMAKE_BUILD_TYPE "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || echo "Unknown")
        print_info "Build type: $build_type"
        print_info "Binary location: ${BUILD_DIR}/${PROJECT_NAME}.elf"
    else
        print_warning "No binary found. Build the project first."
    fi
}

# Function to clean everything
clean_project() {
    print_action "Cleaning build directory..."
    if [[ -d "$BUILD_DIR" ]]; then
        rm -rf "$BUILD_DIR"
        print_success "Clean complete!"
    else
        print_warning "Build directory does not exist."
    fi
}

# Function to show status
status_project() {
    print_action "Project Status"
    if [[ -d "$BUILD_DIR" ]]; then
        print_info "Build directory exists: $BUILD_DIR"
        if [[ -f "${BUILD_DIR}/${PROJECT_NAME}.elf" ]]; then
            print_success "Binary exists: Yes"
            print_info "Last modified: $(stat -c %y "${BUILD_DIR}/${PROJECT_NAME}.elf" 2>/dev/null || echo "Unknown")"
            local build_type=$(grep CMAKE_BUILD_TYPE "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || echo "Unknown")
            print_info "Build type: $build_type"
        else
            print_warning "Binary does not exist"
        fi
    else
        print_warning "Build directory does not exist"
    fi
}

# Function to show help
show_help() {
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Main Options:"
    echo "  -S           Build Release version and flash (default)"
    echo "  -S[SUB]      Build with sub-options"
    echo "  -Q[SUB]      Query operations (status, info)"
    echo "  -D           Debug only (no build)"
    echo "  -C           Clean build directory"
    echo "  -h           Show this help message"
    echo ""
    echo "Sub-options for -S:"
    echo "  -Sr         Build Release only (no flash)"
    echo "  -Sd         Build Debug only (no flash)"
    echo "  -Sf         Flash only (no build)"
    echo "  -Sg         Build Release and flash (same as -S)"
    echo "  -Sdd        Build Debug and debug session"
    echo ""
    echo "Sub-options for -Q:"
    echo "  -Qs         Show project status"
    echo "  -Qi         Show build information"
    echo ""
    echo "Examples:"
    echo "  $0 -S       # Build Release and flash (default)"
    echo "  $0 -Sr      # Build Release only"
    echo "  $0 -Sd      # Build Debug only"
    echo "  $0 -Sf      # Flash only"
    echo "  $0 -Sdd     # Build Debug and debug"
    echo "  $0 -D       # Debug only (no build)"
    echo "  $0 -Qs      # Show status"
    echo "  $0 -Qi      # Show info"
    echo "  $0 -C       # Clean"
}

# Parse main options
case "${1:-"-S"}" in
    "-S")
        # Default: build Release and flash
        print_action "Build Release and flash"
        build_project "Release"
        flash_binary
        ;;
    "-Sr")
        print_action "Build Release only"
        build_project "Release"
        ;;
    "-Sd")
        print_action "Build Debug only"
        build_project "Debug"
        ;;
    "-Sf")
        print_action "Flash only"
        flash_binary
        ;;
    "-Sg")
        print_action "Build Release and flash"
        build_project "Release"
        flash_binary
        ;;
    "-Sdd")
        print_action "Build Debug and debug"
        build_project "Debug"
        debug_project
        ;;
    "-D")
        print_action "Debug only"
        debug_project
        ;;
    "-Qs")
        status_project
        ;;
    "-Qi")
        info_project
        ;;
    "-Q")
        # Default query: show status
        status_project
        ;;
    "-C")
        clean_project
        ;;
    "-h")
        show_help
        ;;
    *)
        print_error "Invalid option: $1"
        echo ""
        show_help
        exit 1
        ;;
esac
