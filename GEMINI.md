# Gemini Project Context: lain-day

## Project Overview

This project is a sophisticated, text-based interactive fiction game inspired by the themes of the anime *Serial Experiments Lain*. The game is developed and played within a terminal environment.

The architecture follows a **data-driven design**, cleanly separating the game engine from the game content:

-   **`game.py`**: A single Python script that acts as the game engine. It parses story files, manages game state, and handles a hybrid user input system.
-   **`story/*.md`**: A directory of Markdown files, each representing a single scene or story beat. The narrative flow is defined by choices embedded in these files.
-   **`*.json` files**: A collection of JSON files that act as the game's database, defining game rules, character state, world layout, and item properties.
-   **`world/`**: A directory structure that simulates an in-game file system, intended for players to explore via terminal commands.
-   **`DESIGN_GOALS.md`**: A roadmap document that outlines planned and aspirational features, serving as a blueprint for future development.

The gameplay is a unique blend of traditional choice-based narrative and a simulated terminal interface, allowing players to navigate the story through both dialogue options and shell-like commands.

## Building and Running

The project is written in pure Python using only standard libraries. No external dependencies are required.

-   **To run the game, execute the following command in your terminal:**
    ```bash
    python game.py
    ```

## Development Conventions

### 1. Data-Driven Design

The core philosophy is to keep logic and data separate.

-   **`game.py`** contains all the logic.
-   **`.md` and `.json`** files contain all the content and state.

This allows for easy modification of the story, items, and world map without altering the engine code.

### 2. Story and Action System

-   Story scenes are written in Markdown files located in the `story/` directory.
-   Player choices are defined using the syntax: `- [Choice Text](action:action_name)`.
-   The `action_name` corresponds to an entry in `actions.json`, which defines the consequence of the choice.
-   **Action Types**: The engine supports various action types, including:
    -   `story_change`: Loads a new story file.
    -   `location_change`: Changes the player's location and loads a new story file.
    -   `unlock_command`: Adds a new command to the player's list of available terminal commands.
    -   `acquire_item`: (Planned) Adds an item to the player's inventory.

### 3. Game State

-   All persistent character state is stored in `character.json`. This includes player stats, location, inventory, and unlocked abilities (commands, CPU features).

### 4. Modular Databases

The game world is defined by a set of modular JSON files:

-   `actions.json`: The "rulebook" connecting story choices to game events.
-   `world_map.json`: Defines all game locations and their connections.
-   `items.json`: A database of all in-game items and their descriptions.
-   `websites.json`: (Planned) A database for the in-game browsable websites.

### 5. Feature Design Process

-   Major new features are first discussed and then documented in `DESIGN_GOALS.md` before implementation. This serves as a collaborative blueprint and roadmap for the project.
