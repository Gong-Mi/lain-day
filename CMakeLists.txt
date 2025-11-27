cmake_minimum_required(VERSION 3.10)

project(lain_day_c VERSION 1.0)

# --- Character Life/Death Options (Compile-time) ---
# Use these options to exclude specific characters and their associated content
# from the build. Defaults to ON (character is included).
option(CHARACTER_ALICE_ALIVE "Include Alice in the game build" ON)
option(CHARACTER_CHISA_ALIVE "Include Chisa in the game build" ON)
option(CHARACTER_FATHER_ALIVE "Include Father in the game build" ON)
option(CHARACTER_FUYUKO_MIRA_ALIVE "Include Fuyuko Mira (Doctor) in the game build" ON)
option(CHARACTER_LAINS_SISTER_MIRA_ALIVE "Include Mira (Lain's Sister) in the game build" ON)

# --- Feature Toggles ---
option(ENABLE_TYPEWRITER_EFFECT "Enable typewriter text rendering effect" OFF)

# --- Compiler Cache ---
option(USE_CCACHE "Use ccache to speed up compilation, if available" ON)
if(USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        message(STATUS "Using ccache to speed up compilation")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_FOUND}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_FOUND}")
    else()
        message(STATUS "ccache not found, continuing without it")
    endif()
endif()

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
    src/flag_system.c
    src/string_table.c
    src/scenes.c
    ${SCENE_SOURCES}
)

# Add compile definitions for character life/death options
target_compile_definitions(lain_day_c PUBLIC
    $<$<BOOL:${CHARACTER_ALICE_ALIVE}>:CHARACTER_ALICE_ALIVE>
    $<$<BOOL:${CHARACTER_CHISA_ALIVE}>:CHARACTER_CHISA_ALIVE>
    $<$<BOOL:${CHARACTER_FATHER_ALIVE}>:CHARACTER_FATHER_ALIVE>
    $<$<BOOL:${CHARACTER_FUYUKO_MIRA_ALIVE}>:CHARACTER_FUYUKO_MIRA_ALIVE>
    $<$<BOOL:${CHARACTER_LAINS_SISTER_MIRA_ALIVE}>:CHARACTER_LAINS_SISTER_MIRA_ALIVE>
)

# Add feature toggle definitions
target_compile_definitions(lain_day_c PUBLIC
    $<$<BOOL:${ENABLE_TYPEWRITER_EFFECT}>:USE_TYPEWRITER_EFFECT>
)

