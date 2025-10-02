# Gemini Project Context: lain-day

## Project Overview

This project is a sophisticated, text-based interactive fiction game inspired by the themes of the anime *Serial Experiments Lain*. The game is developed and played within a terminal environment.

The architecture follows a **data-driven design**, cleanly separating the game engine from the game content:

-   **`game.py`**: A single Python script that acts as the game engine. It parses story files, manages game state, and handles a hybrid user input system.
-   **`story/*.md`**: A directory of Markdown files, each representing a single scene or story beat. The narrative flow is defined by choices embedded in these files, and each file can define its own location via front matter.
-   **`map/`**: A directory-based database for the game world. Each subdirectory represents a location and contains files defining its name, description, points of interest, and connections.
-   **`*.json` files**: A collection of JSON files that act as the game's database, defining game rules (`actions.json`), character state (`character.json`), and item properties (`items.json`).
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

-   **`game.py`** and the **`engine/`** modules contain all the logic.
-   The **`story/`** and **`map/`** directories, along with **`.json`** files, contain all the content and state.

This allows for easy modification of the story, items, and world map without altering the engine code.

### 2. Story and Location System

-   Story scenes are written in Markdown files located in the `story/` directory.
-   **Location Definition**: Each story file can (and should) define the location it takes place in via a YAML-like front matter block at the top of the file.
    ```markdown
    ---
    location: lain_room
    ---
    ```
-   When the engine loads a story file, it automatically reads this metadata and updates the player's current location. This makes all scene transitions seamless.
-   Player choices are defined using the syntax: `- [Choice Text](action:action_name)`.
-   The `action_name` corresponds to an entry in `actions.json`, which defines the consequence of the choice.

### 3. Action System

-   **Action Types**: The engine supports various action types defined in `actions.json`, including:
    -   `story_change`: Loads a new story file. The player's location is automatically updated based on the new file's front matter.
    -   `unlock_command`: Adds a new command to the player's list of available terminal commands.
    -   `acquire_item`: Adds an item to the player's inventory, checking against the player's `credit_level`.
    -   Various composite actions for setting flags (`story_change_and_set_flags`).

### 4. Game State and Inventory

-   All persistent character state is stored in `character.json`.
-   **`location`**: The player's current physical location, used for commands like `arls`.
-   **`credit_level`**: A numeric representation of the player's access/trust level in the world, used for acquiring items.
-   **`inventory`**: A dictionary that tracks owned items and their quantities (e.g., `{"milk": 2}`).

### 5. Modular World Definition

The game world is now defined by a modular directory structure:

-   **`map/`**: The root directory for the world map.
    -   **`map/<location_id>/`**: Each subdirectory represents a single location.
        -   `name.txt`: The display name of the location.
        -   `description.txt`: The description of the location.
        -   `poi.json`: A list of points of interest for the `arls` command.
        -   `connections.json`: Defines connections to other locations.
-   **`actions.json`**: The "rulebook" connecting story choices to game events.
-   **`items.json`**: A database of all in-game items, their descriptions, and their `required_credit`.

---

## Core World-Building and Gameplay Mechanics

This section outlines the core narrative and gameplay loops designed to create a deep, immersive, and mysterious experience inspired by *Serial Experiments Lain*.

### 1. Central Conflict: The Knights of the Eastern Calculus

The primary driving force in the game world is a secretive and powerful hacker collective known as the **Knights of the Eastern Calculus**. Their goal is to build a "Digital Babel"—a unified computing network—to achieve a new level of power and consciousness within the Wired. Their ambitious plan is codenamed **Project Beowulf**.

### 2. Gameplay Philosophy: Narrative-Driven Immersive Sim

The game is designed to avoid repetitive open-world tropes. The core gameplay loop is centered on **mystery, investigation, and experimentation**, not on completing checklists. The player acts as a "digital archaeologist," piecing together the story and the world's hidden rules from fragmented documents and observing the emergent consequences of their actions.

### 3. Key In-Game Technologies & Associated Mechanics

These technologies are introduced to the player via in-game documents and form the basis of the core gameplay mechanics:

-   **Project Beowulf (Unified Computing):**
    -   **Lore:** The Knights' plan to unify all devices under a single, standardized dynamic library architecture.
    -   **Mechanic:** This provides the in-universe justification for other mechanics. It also allows for a **Distributed Execution Environment**, which explains how the player's device can gain massive, temporary performance boosts by leveraging the power of an entire local network.

-   **ToneBurst Protocol (Viral Propagation):**
    -   **Lore:** A worm-like protocol that uses audio signals over telephone lines to broadcast data and infect/upgrade other devices.
    -   **Mechanic:** This translates into a quantifiable **Regional Infection Probability**. The game world becomes a dynamic system where network events can cascade, and the player can witness or even trigger these changes.

-   **MLC Density Doubler (High-Risk Tech):**
    -   **Lore:** An experimental hack to force storage hardware to hold double its rated capacity, at extreme risk of data corruption.
    -   **Mechanic:** Represents the high-risk, high-reward nature of the game's technology, where players can use dangerous, unsanctioned methods to achieve their goals.
