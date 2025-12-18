#!/data/data/com.termux/files/usr/bin/bash

# Ensure Termux tools are in the PATH, even when run via su
export PATH="/data/data/com.termux/files/usr/bin:$PATH"

# Get the project root directory (one level up from this script)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$DIR/.."

# Navigate to project root
cd "$PROJECT_ROOT" || exit 1

# Check if the effective user is root
if [ "$USER" = "root" ]; then
    echo "Error: Please do not run this script as root (\$USER=root)."
    echo "Running build as root can cause file ownership issues."
    exit 1
fi

echo "Starting release build..."

# Clean build
if [ -d "build" ]; then
    rm -rf build
fi
mkdir build
cd build

# Log file for CMake (in case of failure)
CMAKE_LOG="cmake_config.log"

# 1. Configure CMake for Release
#    - CMAKE_BUILD_TYPE=Release enables optimizations.
#    - MASTER_DEBUG_SWITCH=OFF disables all debugging output and features.
cmake -DCMAKE_BUILD_TYPE=Release \
      -DMASTER_DEBUG_SWITCH=OFF \
      .. > "$CMAKE_LOG" 2>&1

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed. See details below:"
    cat "$CMAKE_LOG"
    exit 1
fi

echo "CMake configuration complete. Compiling..."

# 2. Compile (Silent stdout, Show Warnings/Errors only)
make -j$(nproc) > /dev/null

# Check make exit code to inform user if it failed (errors will be visible anyway)
if [ $? -ne 0 ]; then
    echo "Build failed due to errors."
    exit 1
fi

echo "Build successful! Release executable is at: build/lain_day_c"