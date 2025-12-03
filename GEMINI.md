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

## Scene Management Refactoring Strategy

It has been identified that the game's scene management is in a hybrid state. While some scenes are defined programmatically in C code, the `transition_to_scene` function still uses `.md` file paths (e.g., `"story/01_lain_room.md"`) as string identifiers in a dispatch table to map to C-based scene initialization functions. This is a legacy of a previous, incomplete refactoring.

To improve clarity and maintainability without undertaking a massive, immediate refactoring:
*   **Incremental Approach:** We will adopt an incremental refactoring strategy for scene naming.
*   **New Scenes:** New scenes and scene states (e.g., for Mika's Room access) will use clear, C-style identifiers (e.g., `SCENE_MIKA_ROOM_UNLOCKED`, `SCENE_MIKA_ROOM_LOCKED`) as their keys in the `scene_registrations` dispatch table, instead of `.md` file path strings.
*   **Story Content:** The actual story content (dialogue, choices, actions) for these new scenes will be defined directly in their corresponding C source files, replacing the need for separate `.md` files.
*   **Goal:** This approach aims to gradually transition the entire story system to a more robust, C-code-based structure, reducing reliance on string parsing of file paths for scene identification.

## Millennium Crisis Integration Design Considerations

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
    *   **Thematic Time Corruption (14-bit ECC):** For the `time_of_day` variable specifically, a 14-bit integer with ECC (Error-Correcting Code) could be used. If the time value from the save file shows an uncorrectable ECC error (either from player tampering or a simulated Y2K event), this could trigger a *specific in-game event* (e.g., a 'time glitch' narrative sequence, a unique challenge) that aligns with the Y2K theme, rather than just rejecting the save. This allows for both general save file integrity and specific thematic corruption.

