cmake_minimum_required(VERSION 3.10)

project(lain_day_c VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Add executable
add_executable(lain_day_c 
    src/main.c 
    src/cJSON.c 
    src/data_loader.c 
    src/map_loader.c 
    # src/story_parser.c # Temporarily removed for hardcoded scene demo
    src/executor.c
    src/string_table.c
    src/scenes.c
)
