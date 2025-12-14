# Project Overview

This project is an interactive fiction game named `lain-day`, inspired by the anime *Serial Experiments Lain*. The game is written in C and uses a hybrid data-driven and code-driven approach. The project is built using CMake and has dependencies on `cJSON`, `zlib`, and `linenoise`.

The game is played in a terminal and simulates the experience of connecting to and exploring a mysterious network called "the Wired." The gameplay involves making choices, navigating different "layers" of the Wired, and uncovering a branching narrative with multiple endings.

A detailed, real-time list of completed and planned features is maintained in the `include/project_status.h` file.

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
./lain_day_c
```

The game will prompt for a session name. You can enter a new name to start a new game or an existing name to resume a previous game. Game sessions are saved in the `session/` directory.

## Development Status & Conventions

*   **Language:** The project is written in C (C11 standard).
*   **Build System:** CMake is used for building the project.
*   **Design:** The project uses a **hybrid data-driven and code-driven design**. Scene structure, dialogue, and choices are defined in `.ssl` (YAML) files located in `data/scenes/`. A build-time script parses these files and generates C functions to load and manage scene data. Core game logic and action execution are implemented directly in C.
*   **Source of Truth:** For a detailed breakdown of what is implemented, what is in progress, and what is planned, please refer to **`include/project_status.h`**.
*   **Key Changes:**
    *   The `actions.json` file has been **deprecated**. All action logic is now part of the C engine.
    *   The original `map/` directory has been replaced by the `world/` directory system.
*   **Feature Toggles:** The `CMakeLists.txt` file includes several feature toggles for enabling and disabling characters and debug features. This is useful for creating different builds and for testing.
*   **Testing:** The game can be run in an automated mode by providing input as command-line arguments. A `scene_debugger` tool is also available.

## Gameplay and Command System

The game is played through a command-line interface that simulates a shell. Player interaction is driven by a set of commands that evolve as the game progresses.

-   **`arls` (Area List Scan):** The primary command for environmental awareness. It displays the current location's description, a list of "Points of Interest" (POIs), and available "Connections" to other locations.
-   **`exper <id>` (Experiment):** Used to interact with POIs. The `<id>` is discovered through the `arls` command.
-   **`move <destination>`:** The primary command for navigation. The `<destination>` is an `action_id` found in the "Connections" list provided by the `arls` command.
-   **Core Utilities:** Standard commands like `help`, `inventory` (`inv`), and `time` are also available.

## Development Environment and Platform Strategy

### Primary Development Environment

This project is currently being developed and tested primarily within the **Termux environment on Android devices**.

### Platform Strategy

In the current phase, as the narrative content and core framework are not yet fully established, the project adopts a **single-platform-first strategy**. All testing and iteration are concentrated on the Termux environment to maximize development efficiency and minimize the additional burden of multi-platform compatibility.

Multi-platform considerations (e.g., Windows, Linux) will only be addressed once the core narrative and framework are stable. This strategy allows the solo developer to focus more intently on the project's core development.

## Scene Management Architecture and Refactoring Strategy

The game's scene management system is a key component that has evolved significantly. Understanding its current state is crucial for development.

### Current Architecture

The scene transition mechanism operates as follows:

1.  **Scene Definition:** Scenes are defined as `.ssl` files (which are YAML format) in the `data/scenes/` directory. Each file defines the scene's content (dialogue, choices) and contains a unique string identifier, `scene_id` (e.g., `SCENE_01_LAIN_ROOM`).

2.  **Build-Time Code Generation:** At build time, a Python script (`cmake/parse_scenes.py`) processes all `.ssl` files. For each scene, it automatically generates a corresponding C function (e.g., `init_scene_SCENE_01_LAIN_ROOM_from_data`).

3.  **Dispatch Table:** The `src/scenes.c` file contains a static dispatch table named `scene_registrations`. This table maps the string `scene_id` from the `.ssl` files to the function pointers of the auto-generated C functions.

4.  **Scene Transition:** A transition is initiated when a new `scene_id` is written to the `game_state->current_story_file` field. Note that the field name `current_story_file` is a misnomer from a previous design; it holds a `scene_id`, not a file path. The main game loop detects this change and calls `transition_to_scene`, which uses the `scene_id` to look up the correct function in the dispatch table and load the new scene.

5.  **Conditional Choices:** The visibility of choices within a scene is managed by a flexible condition system. In the `.ssl` files, each choice can have a `conditions` list. At build time, `parse_scenes.py` converts this list into a C array of `Condition` structs. At runtime, the `is_choice_selectable` function calls `check_conditions` to evaluate this array, allowing for complex logic based on game days, time of day, and story flags. This avoids runtime parsing and provides high performance.

### Refactoring Strategy and Development Philosophy

#### The Role of 'Fragile' Identifiers (Fail-Fast Principle)

A key development philosophy for this project is "Fail-Fast." This principle is particularly important for a solo developer, as it minimizes the costly context switch of debugging issues long after they were introduced.

The current reliance on string identifiers (`scene_id`) for scene dispatch is a deliberate implementation of this philosophy. The system is intentionally "fragile" in that a misspelled or non-existent `scene_id` should cause an immediate and loud failure during development. This provides instant feedback, allowing errors to be caught and fixed within the creative flow, rather than being discovered later during a separate testing phase. This is considered a feature for development efficiency, not a bug.

#### Long-Term Refactoring Goal

While the fail-fast approach is crucial during development, the long-term goal for the *runtime* is to enhance robustness and maintainability. Therefore, the refactoring strategy aims to achieve the best of both worlds:

*   **Incremental Transition to Enums:** Gradually introduce C-style enumerations (`enum`) generated from the `scene_id`s. This will provide compile-time safety and improve code-completion and readability, reducing the chance of identifier errors in the first place.
*   **Preserve Fail-Fast in Debug Builds:** The scene lookup function, even when using enums, should retain its strictness. In debug builds, an invalid scene identifier should still trigger an immediate assertion or crash, preserving the rapid feedback loop.
*   **Goal:** To evolve the scene management system to be compile-time safe and highly readable through enums, while strictly enforcing the fail-fast discipline during development builds to maximize solo developer efficiency.

### Map and Location Architecture

The game's world map is constructed programmatically at runtime, moving away from a previous placeholder system.

1.  **Location Definition:** The canonical definition for game locations (e.g., Lain's room, the street, parks) is now done in dedicated C files, typically named `scene.c`, located within subdirectories of the `sequences/` directory (e.g., `sequences/miyanosaka/iwakura_house/scene.c`). Each of these files contains a function (e.g., `create_iwakura_house_layout`) that programmatically defines one or more `Location` structs.

2.  **Central Loader:** The main `load_map_data` function in `src/map_loader.c` acts as the central aggregator. It is responsible for calling the various `create_*_layout` functions from the `sequences` directory to collect all locations and populate the final world map.

3.  **POIs and Connections:** Within each location's definition, Points of Interest (POIs) and Connections to other locations are added programmatically using the `add_poi_to_location` and `add_connection_to_location` helper functions. These connections use action IDs (e.g., `go_to_park`, `use_desktop_navi`) which are then handled by the `executor.c` module.

4.  **Deprecated Placeholder System:** The function `load_programmatic_map_data` within `src/map_loader.c` previously contained hardcoded placeholder locations. This system is now **fully deprecated and has been gutted**. All locations are now defined through the more modular `sequences` system to avoid data duplication and logical errors.

## Train System Integration (Work in Progress)

The game will feature an interactive train system to facilitate travel across Tokyo. This system is implemented as a C-based module, offering a more dynamic and interactive experience than simple scene transitions.

### Current Status: Ticket Machine Placeholder

*   **Interface**: A `train_system` module (`src/systems/train_system.c`) has been introduced. When activated, it presents a "ticket machine" interface.
*   **Functionality**: This interface lists all available train stations (loaded from `sequences/station_coordinates.json`) and allows the player to select a destination. Currently, only an "exit" option is functional, returning the player to their previous location.
*   **Entry Point**: Each train station location will include a "ticket machine" Point of Interest (POI). Interacting with this POI (`exper ticket_machine`) will launch the `enter_ticket_machine_interface`.

### Future Considerations and Planned Features

*   **Programmatic Waiting**: Implement a system where players must wait for a train to arrive at a station.
*   **Time Progression**: Integrate time advancement based on travel duration between stations.
*   **Missing Stops**: Develop mechanics where players can accidentally miss their intended stop, leading to new challenges or narrative branches.
*   **Complex Interactions**: Expand the train system to include various events, characters, or mini-games during transit.
*   **Visual Representation**: Explore options for a more visual or animated representation of the train journey within the terminal.

## Millennium Crisis Integration Design Considerations

**NOTE: The following section describes a design concept for a potential future feature. It has NOT been implemented.**

To integrate the 'Millennium crisis' (Y2K crisis and Millennium Digital Act) feeling into the game, leveraging its C-based, data-driven interactive fiction nature, the following approaches are being considered:

### 1. Narrative Integration
*   **New Story Scenes:** Create programmatic story scenes (similar to C-based scenes) that describe Y2K-related events, such as news reports, character discussions, or Lain's experiences with glitches/system failures in the Wired.
*   **Existing Scene Modification:** Conditionally inject Y2K-related dialogue or descriptions into existing scenes based on game flags.

### 2. Flag System for Game State
*   **Crisis Flags:** Introduce new game flags (e.g., `Y2K_ACTIVE`, `MILLENNIUM_ACT_IMPACT`, `WIRED_GLITCH_LEVEL`) to track the state and severity of the crisis.
*   **Conditional Events:** Use these flags to trigger different story branches, dialogue, or affect gameplay mechanics (e.g., `NAVI` unreliability, network surveillance, content restrictions).

### 3. Time System Interaction
*   **Event Triggers:** Trigger Y2K-related events on specific in-game dates or times.
*   **Time Advancement:** Player choices related to the crisis could fast-forward time to critical moments.

### 4. Action System
*   **Crisis-Specific Actions:** Introduce new actions (e.g., `check_news_feeds`, `secure_navi`, `bypass_restrictions`) that become available during the crisis.
*   **Consequences:** Outcomes of these actions would be influenced by the crisis flags.

### 5. Data Integrity and Thematic Corruption
The discussion explored preventing save file manipulation and integrating the millennium crisis theme:

*   **Player Save File Manipulation:** Players can potentially modify `character.json` to bypass in-game protection.
*   **Proposed Solutions:**
    *   **Save File Checksum/Hashing:** Implement a checksum/hash for `character.json` to detect any tampering upon loading. This provides general save file integrity.
    *   **Thematic Time Corruption and Obfuscation:** The `time_of_day` variable is a 32-bit integer with a special structure designed for data integrity and obfuscation. The lower 30 bits constitute an ECC (Error-Correcting Code) codeword, which protects 24 bits of actual time data with 6 bits of validation data (SECDED). The top 2 bits of the 32-bit integer are intentionally ignored by the time-advancement logic, acting as random "noise" to complicate save-file tampering. The 24-bit limit on the time data itself creates a natural cycle of approximately 12 game days, which is a core mechanic of the game's time progression. If the game detects an uncorrectable error in the time data upon loading, it can trigger specific in-game "time glitch" events.
