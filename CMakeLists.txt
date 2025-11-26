cmake_minimum_required(VERSION 3.10)

project(lain_day_c VERSION 1.0)

# --- Platform/Architecture Information ---
# Note: Specific detection for Termux is deferred.
# If generic Android detection causes issues in Termux,
# check for TERMUX_VERSION environment variable here.
# For example:
# if(DEFINED ENV{TERMUX_VERSION})
#     set(PLATFORM_NAME "Termux")
# else()
#     set(PLATFORM_NAME ${CMAKE_SYSTEM_NAME})
# endif()
# message(STATUS "Detected Platform: ${PLATFORM_NAME}")

# Print detected info to the console during configuration
message(STATUS "Detected OS: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_VERSION})")
message(STATUS "Detected Arch: ${CMAKE_SYSTEM_PROCESSOR}")

# Generate a header file with the detected platform info
configure_file(
    "${PROJECT_SOURCE_DIR}/include/build_info.h.in"
    "${PROJECT_BINARY_DIR}/include/build_info.h"
)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)
include_directories(scenes) # Add scenes directory to include path
include_directories("${PROJECT_BINARY_DIR}/include") # For generated headers

# Find all scene source files automatically.
# This allows new scenes to be added simply by creating a new directory
# under /scenes/ with a scene.c file, without needing to modify this file.
file(GLOB_RECURSE SCENE_SOURCES "scenes/*/scene.c")

# Add executable
add_executable(lain_day_c 
    src/main.c 
    src/cJSON.c 
    src/data_loader.c 
    src/map_loader.c 
    src/executor.c
    src/string_table.c
    src/scenes.c
    ${SCENE_SOURCES}
)
