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
option(ENABLE_FULL_GRAPHICS "Enable full-screen graphical image viewer using ANSI/RGB" ON)

if(ENABLE_FULL_GRAPHICS)
    add_definitions(-DENABLE_FULL_GRAPHICS)
endif()

add_definitions(-DUSE_UTF8)

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
option(ENABLE_STRING_DEBUG_LOGGING "Enable debug logging for string IDs to stderr" ON)
# ENABLE_MAP_DEBUG_LOGGING:
# Controls detailed debug messages during map loading and processing (e.g., programmatic location additions, POI verification).
# Useful for debugging world structure and navigation.
# Only effective if MASTER_DEBUG_SWITCH is ON.
# Default: OFF (to keep console output clean during normal development).
option(ENABLE_MAP_DEBUG_LOGGING "Enable debug logging for map loading and processing to stderr" ON)
# ENABLE_CLEAR_SCREEN:
# Controls whether the terminal screen is cleared on each scene render.
# When MASTER_DEBUG_SWITCH is ON, this option functions as a game style toggle.
# When MASTER_DEBUG_SWITCH is OFF, this option serves as a debugging aid (keeping previous output for inspection).
# Only effective if MASTER_DEBUG_SWITCH is ON.
# Default: OFF (keeps previous output, useful for debugging by default).
option(ENABLE_CLEAR_SCREEN "Enable clearing screen on each scene render" ON)

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
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Os")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Include directories
include_directories(include)
include_directories(include/systems) # Add systems directory to include path
include_directories(scenes) # Add scenes directory to include path
include_directories(sequences) # Add sequences directory to include path
include_directories("${PROJECT_BINARY_DIR}/include") # For generated headers
include_directories(external/linenoise) # Add linenoise include path
include_directories(external/cJSON) # Add cJSON include path


# --- Auto-generate character.json data header ---
set(CHARACTER_JSON_FILE "${PROJECT_SOURCE_DIR}/data/character.json")
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

# --- Auto-generate items.json data header ---
set(ITEMS_JSON_FILE "${PROJECT_SOURCE_DIR}/data/items.json")
set(GENERATED_ITEMS_DATA_H "${PROJECT_BINARY_DIR}/include/items_data.h")

add_custom_command(
    OUTPUT ${GENERATED_ITEMS_DATA_H}
    COMMAND ${CMAKE_COMMAND} 
        -DINPUT_FILE=${ITEMS_JSON_FILE} 
        -DOUTPUT_FILE=${GENERATED_ITEMS_DATA_H} 
        -DJSON_VAR_NAME=ITEMS_JSON_DATA
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_json_header.cmake
    DEPENDS ${ITEMS_JSON_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_json_header.cmake
    COMMENT "Generating C header from items.json"
)

add_custom_target(generate_items_header ALL DEPENDS ${GENERATED_ITEMS_DATA_H})

# --- Auto-generate station_coordinates.json data header ---
set(STATION_COORDINATES_JSON_FILE "${PROJECT_SOURCE_DIR}/sequences/station_coordinates.json")
set(GENERATED_STATION_DATA_H "${PROJECT_BINARY_DIR}/include/station_coordinates_data.h")

add_custom_command(
    OUTPUT ${GENERATED_STATION_DATA_H}
    COMMAND ${CMAKE_COMMAND} 
        -DINPUT_FILE=${STATION_COORDINATES_JSON_FILE} 
        -DOUTPUT_FILE=${GENERATED_STATION_DATA_H} 
        -DJSON_VAR_NAME=STATION_COORDINATES_JSON_DATA
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_json_header.cmake
    DEPENDS ${STATION_COORDINATES_JSON_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_json_header.cmake
    COMMENT "Generating C header from station_coordinates.json"
)

add_custom_target(generate_station_data_header ALL DEPENDS ${GENERATED_STATION_DATA_H})

# --- Auto-generate string_ids.h from strings.json and strings_extra/*.json ---
set(STRINGS_JSON_FILE "${PROJECT_SOURCE_DIR}/data/strings.json")
file(GLOB STRINGS_EXTRA_FILES
    "${PROJECT_SOURCE_DIR}/data/strings_extra/*.json"
    "${PROJECT_SOURCE_DIR}/data/strings_extra/generated_placeholders.json"
)

set(GENERATED_STRINGS_H "${PROJECT_BINARY_DIR}/include/string_ids.h")
set(GENERATED_STRINGS_NAMES_H "${PROJECT_BINARY_DIR}/include/string_id_names.h")
set(GENERATED_STRINGS_NAMES_C "${PROJECT_BINARY_DIR}/src/generated_string_id_names.c")
set(GENERATED_STRINGS_DATA_C "${PROJECT_BINARY_DIR}/src/generated_string_data.c")

add_custom_command(
    OUTPUT ${GENERATED_STRINGS_H} ${GENERATED_STRINGS_NAMES_H} ${GENERATED_STRINGS_NAMES_C} ${GENERATED_STRINGS_DATA_C}
    COMMAND /data/data/com.termux/files/usr/bin/python3.12 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_string_ids.py 
        ${GENERATED_STRINGS_H} 
        ${GENERATED_STRINGS_NAMES_H} 
        ${GENERATED_STRINGS_NAMES_C} 
        ${GENERATED_STRINGS_DATA_C}
        ${STRINGS_JSON_FILE} 
        ${STRINGS_EXTRA_FILES}
    DEPENDS ${STRINGS_JSON_FILE} ${STRINGS_EXTRA_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_string_ids.py
    COMMENT "Generating C headers and source from strings.json and extra modules"
)

add_custom_target(generate_string_ids_header ALL DEPENDS ${GENERATED_STRINGS_H} ${GENERATED_STRINGS_NAMES_H} ${GENERATED_STRINGS_NAMES_C} ${GENERATED_STRINGS_DATA_C})

# Find all scene source files automatically.

file(GLOB_RECURSE SCENE_SUBDIR_SOURCES "scenes/*/scene.c")

file(GLOB SCENE_ROOT_SOURCES "scenes/*.c")

# --- Auto-generate scene C headers and sources from SSL files ---
file(GLOB_RECURSE SSL_SOURCE_FILES "${PROJECT_SOURCE_DIR}/data/scenes/*.ssl")

set(GENERATED_SSL_SCENE_HEADERS "")
set(GENERATED_SSL_SCENE_SOURCES "")

foreach(ssl_file ${SSL_SOURCE_FILES})
    get_filename_component(file_name ${ssl_file} NAME_WE) # e.g., "00_entry"
    string(TOUPPER ${file_name} SCENE_ID_UPPER) # e.g., "00_ENTRY"
    string(REPLACE "-" "_" SCENE_ID_UPPER ${SCENE_ID_UPPER}) # Replace hyphens for C identifier

    # Generated header path (e.g., build/include/SCENE_00_ENTRY_data.h)
    set(GENERATED_SCENE_H "${PROJECT_BINARY_DIR}/include/SCENE_${SCENE_ID_UPPER}_data.h")
    # Generated C source path (e.g., build/src/SCENE_00_ENTRY_data.c)
    set(GENERATED_SCENE_C "${PROJECT_BINARY_DIR}/src/SCENE_${SCENE_ID_UPPER}_data.c")

    list(APPEND GENERATED_SSL_SCENE_HEADERS ${GENERATED_SCENE_H})
    list(APPEND GENERATED_SSL_SCENE_SOURCES ${GENERATED_SCENE_C})

    # Add custom command for each SSL file
    add_custom_command(
        OUTPUT ${GENERATED_SCENE_H} ${GENERATED_SCENE_C}
        COMMAND /data/data/com.termux/files/usr/bin/python3.12 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/parse_scenes.py
            ${ssl_file}
            ${PROJECT_BINARY_DIR}/include/string_ids.h # Pass string_ids.h for validation
            ${GENERATED_SCENE_H}
            ${GENERATED_SCENE_C}
        DEPENDS ${ssl_file} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/parse_scenes.py ${PROJECT_BINARY_DIR}/include/string_ids.h
        COMMENT "Generating C scene data from ${file_name}.ssl"
    )
endforeach()

add_custom_target(generate_ssl_scenes ALL DEPENDS ${GENERATED_SSL_SCENE_HEADERS} ${GENERATED_SSL_SCENE_SOURCES})

# Find all plot sequence source files automatically.
file(GLOB_RECURSE SEQUENCE_SOURCES "sequences/*/scene.c")

# --- Define common source files for the game engine ---

set(GAME_ENGINE_SOURCES

    external/cJSON/cJSON.c

    src/data_loader.c

    src/map_loader.c

        src/game_state_global.c

        src/executor.c

        src/flag_system.c

        src/string_table.c

        src/scenes.c

        src/event_system.c

        src/cmap.c

        src/conditions.c

        src/time_utils.c

        src/ecc_time.c

        src/compression_util.c

        src/render_utils.c

        src/game_paths.c
        src/logger.c

        src/systems/embedded_navi.c
        src/systems/mail_system.c # New: Add mail system source file

        src/systems/navi_mini.c

        src/systems/navi_pro.c

        src/systems/navi_alpha.c
        src/systems/train_system.c
        src/systems/mystery_system.c
        src/systems/image_view_system.c
        src/systems/navi_shell.c # Add shell system
        src/systems/boot_system.c # Add boot system

        external/linenoise/linenoise.c
    external/linenoise/stringbuf.c
    external/linenoise/utf8.c

        ${SEQUENCE_SOURCES}

        src/characters/mika.c
        ${GENERATED_STRINGS_NAMES_C}
        ${GENERATED_STRINGS_DATA_C}
        ${GENERATED_SSL_SCENE_SOURCES} # Add generated SSL scene sources

    )

# --- Define Executables ---

# Main game executable
add_executable(lain_day_c src/main.c ${GAME_ENGINE_SOURCES})
target_link_libraries(lain_day_c PUBLIC zlibstatic pthread)

# Scene debugger tool
add_executable(scene_debugger tools/scene_debugger.c ${GAME_ENGINE_SOURCES})
target_link_libraries(scene_debugger PUBLIC zlibstatic pthread)

# NAVI debugger tool
add_executable(navi_debugger tools/navi_debugger.c ${GAME_ENGINE_SOURCES})
target_link_libraries(navi_debugger PUBLIC zlibstatic pthread)

# Map debugger tool
add_executable(map_debugger tools/map_debugger.c ${GAME_ENGINE_SOURCES})
target_link_libraries(map_debugger PUBLIC zlibstatic pthread)

# Mika Schedule debugger tool
add_executable(debug_mika_schedule tools/debug_mika_schedule.c ${GAME_ENGINE_SOURCES})
target_link_libraries(debug_mika_schedule PUBLIC zlibstatic pthread)

# Boot Sequence debugger tool
add_executable(boot_debugger tools/boot_debugger.c ${GAME_ENGINE_SOURCES})
target_link_libraries(boot_debugger PUBLIC zlibstatic pthread)

# Add dependency to ensure header is generated before compiling executables
add_dependencies(lain_day_c generate_character_header generate_items_header generate_string_ids_header generate_ssl_scenes generate_station_data_header)
add_dependencies(scene_debugger generate_character_header generate_items_header generate_string_ids_header generate_ssl_scenes)
add_dependencies(navi_debugger generate_string_ids_header)
add_dependencies(map_debugger generate_string_ids_header)
add_dependencies(debug_mika_schedule generate_string_ids_header)
add_dependencies(boot_debugger generate_string_ids_header generate_character_header)

# Add feature toggle definitions
# The following compile definitions (USE_TYPEWRITER_EFFECT, USE_DEBUG_LOGGING, etc.)
# are enabled based on their respective CMake options, but are also dependent
# on MASTER_DEBUG_SWITCH being ON. This means if MASTER_DEBUG_SWITCH is OFF,
# none of these features will be enabled, regardless of their individual settings.
target_compile_definitions(scene_debugger PUBLIC
    $<$<BOOL:${MASTER_DEBUG_SWITCH}>:$<$<BOOL:${ENABLE_DEBUG_LOGGING}>:USE_DEBUG_LOGGING>>
    $<$<BOOL:${MASTER_DEBUG_SWITCH}>:$<$<BOOL:${ENABLE_STRING_DEBUG_LOGGING}>:USE_STRING_DEBUG_LOGGING>>
    $<$<BOOL:${MASTER_DEBUG_SWITCH}>:$<$<BOOL:${ENABLE_MAP_DEBUG_LOGGING}>:USE_MAP_DEBUG_LOGGING>>
)


# Add compile definitions for character life/death options
target_compile_definitions(lain_day_c PUBLIC
    $<$<BOOL:${MASTER_DEBUG_SWITCH}>:$<$<BOOL:${ENABLE_DEBUG_LOGGING}>:USE_DEBUG_LOGGING>>
    $<$<BOOL:${MASTER_DEBUG_SWITCH}>:$<$<BOOL:${ENABLE_STRING_DEBUG_LOGGING}>:USE_STRING_DEBUG_LOGGING>>
    $<$<BOOL:${MASTER_DEBUG_SWITCH}>:$<$<BOOL:${ENABLE_MAP_DEBUG_LOGGING}>:USE_MAP_DEBUG_LOGGING>>
    $<$<BOOL:${ENABLE_CLEAR_SCREEN}>:USE_CLEAR_SCREEN>
    $<$<BOOL:${CHARACTER_ALICE_ALIVE}>:CHARACTER_ALICE_ALIVE>
    $<$<BOOL:${CHARACTER_CHISA_ALIVE}>:CHARACTER_CHISA_ALIVE>
    $<$<BOOL:${CHARACTER_FATHER_ALIVE}>:CHARACTER_FATHER_ALIVE>
    $<$<BOOL:${CHARACTER_FUYUKO_MIRA_ALIVE}>:CHARACTER_FUYUKO_MIRA_ALIVE>
    $<$<BOOL:${CHARACTER_LAINS_SISTER_MIRA_ALIVE}>:CHARACTER_LAINS_SISTER_MIRA_ALIVE>
)

# --- Installation Rules ---
install(TARGETS lain_day_c DESTINATION bin)
install(FILES 
    data/character.json
    items.json
    actions.json
    DESTINATION share/${PROJECT_NAME}
)
install(DIRECTORY map/ DESTINATION share/${PROJECT_NAME}/map PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY data/story/ DESTINATION share/${PROJECT_NAME}/story PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)
install(DIRECTORY world/ DESTINATION share/${PROJECT_NAME}/world PATTERN ".*" EXCLUDE PATTERN "*~" EXCLUDE)


