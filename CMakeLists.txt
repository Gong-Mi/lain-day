cmake_minimum_required(VERSION 3.10)

project(lain_day_c VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)
include_directories(scenes) # Add scenes directory to include path

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
