cmake_minimum_required(VERSION 3.10)

project(lain_day_c VERSION 1.0)

add_subdirectory(external/zlib)

# --- Character Life/Death Options (Compile-time) ---
# Use these options to exclude specific characters and their associated content
# from the build. Defaults to ON (character is included).
option(CHARACTER_ALICE_ALIVE "Include Alice in the game build" ON)
option(CHARACTER_CHISA_ALIVE "Include Chisa in the game build" ON)
option(CHARACTER_FATHER_ALIVE "Include Father in the game build" ON)
option(CHARACTER_FUYUKO_MIRA_ALIVE "Include Fuyuko Mira (Doctor) in the game build" ON)
option(CHARACTER_LAINS_SISTER_MIRA_ALIVE "Include Mira (Lain's Sister) in the game build" ON)

# --- Feature Toggles ---
# MASTER_DEBUG_SWITCH:
# This is the master switch for all debug-related options.
# If set to OFF, all other debug-specific toggles below will be ignored,
# effectively disabling all debug output.
# If set to ON, the individual debug toggles (ENABLE_DEBUG_LOGGING, etc.) will be respected.
# Default: ON (allowing individual toggles to be used or providing a quick way to disable all debugs).
option(MASTER_DEBUG_SWITCH "Master switch for all debug logging" ON)
# ENABLE_TYPEWRITER_EFFECT:
# Controls the typewriter text rendering effect.
# When MASTER_DEBUG_SWITCH is ON, this toggle determines if text is printed character by character.
# Default: OFF (faster text display).
option(ENABLE_TYPEWRITER_EFFECT "Enable typewriter text rendering effect" OFF)
# ENABLE_DEBUG_LOGGING:
# Controls general debug log messages printed to stderr.
# These messages provide insight into core game logic and data loading.
# Only effective if MASTER_DEBUG_SWITCH is ON.
# Default: ON (useful for general development tracking).
option(ENABLE_DEBUG_LOGGING "Enable debug logging to stderr" ON)
# ENABLE_STRING_DEBUG_LOGGING:
# Controls detailed debug messages related to String IDs and their content during scene rendering.
# Useful for debugging text display and internationalization issues.
# Only effective if MASTER_DEBUG_SWITCH is ON.
# Default: OFF (to keep console output clean during normal development).
option(ENABLE_STRING_DEBUG_LOGGING "Enable debug logging for string IDs to stderr" OFF)
# ENABLE_MAP_DEBUG_LOGGING:
# Controls detailed debug messages during map loading and processing (e.g., programmatic location additions, POI verification).
# Useful for debugging world structure and navigation.
# Only effective if MASTER_DEBUG_SWITCH is ON.
# Default: OFF (to keep console output clean during normal development).
option(ENABLE_MAP_DEBUG_LOGGING "Enable debug logging for map loading and processing to stderr" OFF)
# ENABLE_CLEAR_SCREEN:
# Controls whether the terminal screen is cleared on each scene render.
# When MASTER_DEBUG_SWITCH is ON, this option functions as a game style toggle.
# When MASTER_DEBUG_SWITCH is OFF, this option serves as a debugging aid (keeping previous output for inspection).
# Only effective if MASTER_DEBUG_SWITCH is ON.
# Default: OFF (keeps previous output, useful for debugging by default).
option(ENABLE_CLEAR_SCREEN "Enable clearing screen on each scene render" OFF)

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
include_directories(sequences) # Add sequences directory to include path
include_directories("${PROJECT_BINARY_DIR}/include") # For generated headers

# --- Auto-generate character.json data header ---
set(CHARACTER_JSON_FILE "${PROJECT_SOURCE_DIR}/character.json")
set(GENERATED_CHARACTER_DATA_H "${PROJECT_BINARY_DIR}/include/character_data.h")

# Add custom command to generate the header file
add_custom_command(
    OUTPUT ${GENERATED_CHARACTER_DATA_H}
    COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${CHARACTER_JSON_FILE} -DOUTPUT_FILE=${GENERATED_CHARACTER_DATA_H} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_json_header.cmake
    DEPENDS ${CHARACTER_JSON_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_json_header.cmake
    COMMENT "Generating C header from character.json"
)

# Create a target for the generated header to ensure it's built
add_custom_target(generate_character_header ALL DEPENDS ${GENERATED_CHARACTER_DATA_H})

# Find all scene source files automatically.

file(GLOB_RECURSE SCENE_SUBDIR_SOURCES "scenes/*/scene.c")

file(GLOB SCENE_ROOT_SOURCES "scenes/*.c")



# Find all plot sequence source files automatically.

file(GLOB_RECURSE SEQUENCE_SOURCES "sequences/*/scene.c")



# --- Define common source files for the game engine ---

set(GAME_ENGINE_SOURCES

    src/cJSON.c

    src/data_loader.c

    src/map_loader.c

    src/executor.c

    src/flag_system.c

    src/string_table.c

    src/scenes.c

    src/cmap.c

    src/conditions.c

    src/time_utils.c

    src/compression_util.c

    src/render_utils.c

    ${SCENE_SUBDIR_SOURCES}

    ${SCENE_ROOT_SOURCES}

    ${SEQUENCE_SOURCES}

)

# --- Define Executables ---

# Main game executable
add_executable(lain_day_c src/main.c ${GAME_ENGINE_SOURCES})
target_link_libraries(lain_day_c PUBLIC zlibstatic pthread)

# Scene debugger tool
add_executable(scene_debugger tools/scene_debugger.c ${GAME_ENGINE_SOURCES})
target_link_libraries(scene_debugger PUBLIC zlibstatic pthread)

# Add dependency to ensure header is generated before compiling executables
add_dependencies(lain_day_c generate_character_header)
add_dependencies(scene_debugger generate_character_header)

# Add feature toggle definitions
# The following compile definitions (USE_TYPEWRITER_EFFECT, USE_DEBUG_LOGGING, etc.)
# are enabled based on their respective CMake options, but are also dependent
# on MASTER_DEBUG_SWITCH being ON. This means if MASTER_DEBUG_SWITCH is OFF,
# none of these features will be enabled, regardless of their individual settings.
target_compile_definitions(scene_debugger PUBLIC
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_TYPEWRITER_EFFECT}>>:USE_TYPEWRITER_EFFECT>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_DEBUG_LOGGING}>>:USE_DEBUG_LOGGING>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_STRING_DEBUG_LOGGING}>>:USE_STRING_DEBUG_LOGGING>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_MAP_DEBUG_LOGGING}>>:USE_MAP_DEBUG_LOGGING>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_CLEAR_SCREEN}>>:USE_CLEAR_SCREEN>
)


# Add compile definitions for character life/death options
target_compile_definitions(lain_day_c PUBLIC
    $<$<BOOL:${CHARACTER_ALICE_ALIVE}>:CHARACTER_ALICE_ALIVE>
    $<$<BOOL:${CHARACTER_CHISA_ALIVE}>:CHARACTER_CHISA_ALIVE>
    $<$<BOOL:${CHARACTER_FATHER_ALIVE}>:CHARACTER_FATHER_ALIVE>
    $<$<BOOL:${CHARACTER_FUYUKO_MIRA_ALIVE}>:CHARACTER_FUYUKO_MIRA_ALIVE>
    $<$<BOOL:${CHARACTER_LAINS_SISTER_MIRA_ALIVE}>:CHARACTER_LAINS_SISTER_MIRA_ALIVE>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_TYPEWRITER_EFFECT}>>:USE_TYPEWRITER_EFFECT>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_DEBUG_LOGGING}>>:USE_DEBUG_LOGGING>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_STRING_DEBUG_LOGGING}>>:USE_STRING_DEBUG_LOGGING>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_MAP_DEBUG_LOGGING}>>:USE_MAP_DEBUG_LOGGING>
    $<$<AND:$<BOOL:${MASTER_DEBUG_SWITCH}>,$<BOOL:${ENABLE_CLEAR_SCREEN}>>:USE_CLEAR_SCREEN>
)

# --- Installation Rules ---
install(TARGETS lain_day_c DESTINATION bin)
install(FILES 
    character.json
    items.json
    actions.json
    DESTINATION share/${PROJECT_NAME}
)
install(DIRECTORY map/ DESTINATION share/${PROJECT_NAME}/map PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY story/ DESTINATION share/${PROJECT_NAME}/story PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY world/ DESTINATION share/${PROJECT_NAME}/world PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)


