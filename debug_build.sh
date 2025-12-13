#!/data/data/com.termux/files/usr/bin/bash

# Ensure Termux tools are in the PATH, even when run via su
export PATH="/data/data/com.termux/files/usr/bin:$PATH"

# Check if the effective user is root
if [ "$USER" = "root" ]; then
    echo "Error: Please do not run this script as root (\$USER=root)."
    echo "Running build as root can cause file ownership issues."
    exit 1
fi

# Clean build
if [ -d "build" ]; then
    rm -rf build
fi
mkdir build
cd build

# Log file for CMake (in case of failure)
CMAKE_LOG="cmake_config.log"

# 1. Configure CMake (Silent stdout, keep stderr for serious issues, log details to file)
# We capture stdout to a log file so we can show it ONLY if it fails.
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DMASTER_DEBUG_SWITCH=ON \
      -DENABLE_DEBUG_LOGGING=ON \
      -DENABLE_STRING_DEBUG_LOGGING=ON \
      -DENABLE_MAP_DEBUG_LOGGING=ON \
      .. > "$CMAKE_LOG" 2>&1

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed. See details below:"
    cat "$CMAKE_LOG"
    exit 1
fi

# 2. Compile (Silent stdout, Show Warnings/Errors only)
# We redirect stdout (fd 1) to /dev/null to hide "[ 10%] Building..."
# We leave stderr (fd 2) alone, so compiler warnings and errors still appear on screen.
make -j$(nproc) > /dev/null

# Check make exit code to inform user if it failed (errors will be visible anyway)
if [ $? -ne 0 ]; then
    echo "Build failed due to errors."
    exit 1
fi

# If we get here, it means success.
# The user requested "no warnings -> finish". 
# If there were no warnings on stderr, this will be a completely silent run.