# Project Overview

This project is an interactive fiction game named `lain-day`, inspired by the anime *Serial Experiments Lain*. The game is written in C and uses a data-driven approach, with game content defined in `.md` and `.json` files. The project is built using CMake and has dependencies on `cJSON` and `zlib`.

The game is played in a terminal and simulates the experience of connecting to and exploring a mysterious network called "the Wired." The gameplay involves making choices, navigating different "layers" of the Wired, and uncovering a branching narrative with multiple endings.

The project has a strong design vision, with plans for advanced features like a CPU simulation for puzzle-solving, a hybrid media narrative system, and a dynamic network ecosystem.

## Building and Running

To build and run the project, follow these steps:

```bash
# 1. Create and enter the build directory
mkdir -p build
cd build

# 2. Run CMake to generate the build files
cmake ..

# 3. Compile the project
make

# 4. Run the game
./lain-day
```

The game will prompt for a session name. You can enter a new name to start a new game or an existing name to resume a previous game. Game sessions are saved in the `session/` directory.

## Development Conventions

*   **Language:** The project is written in C (C11 standard).
*   **Build System:** CMake is used for building the project.
*   **Coding Style:** The code follows a consistent style, with clear and well-documented functions.
*   **Data-Driven Design:** Game content is separated from the engine and is defined in `.md` and `.json` files. This allows for easy modification and expansion of the game's story and content.
*   **Feature Toggles:** The `CMakeLists.txt` file includes several feature toggles for enabling and disabling characters and debug features. This is useful for creating different builds and for testing specific parts of the game.
*   **Testing:** The game can be run in an automated mode by providing input as command-line arguments. This is useful for testing specific scenarios and for debugging.
