import json
import os
import sys
import shutil

# --- Engine Imports ---
from engine.parser import parse_story_file
from engine.renderer import render_scene, typewriter_print
from engine.terminal import process_command
from engine.game_logic import process_choice

# --- Core Helpers ---
def load_json(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        return None

def save_json(file_path, data):
    with open(file_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

def load_world_map_from_folders(abs_path):
    world_map = {}
    map_root = os.path.join(abs_path, 'map')
    if not os.path.exists(map_root):
        return {}

    for location_id in os.listdir(map_root):
        location_path = os.path.join(map_root, location_id)
        if os.path.isdir(location_path):
            try:
                with open(os.path.join(location_path, 'name.txt'), 'r', encoding='utf-8') as f:
                    name = f.read().strip()
                with open(os.path.join(location_path, 'description.txt'), 'r', encoding='utf-8') as f:
                    description = f.read().strip()
                
                connections = load_json(os.path.join(location_path, 'connections.json')) or {}
                poi = load_json(os.path.join(location_path, 'poi.json')) or []

                world_map[location_id] = {
                    'name': name,
                    'description': description,
                    'connections': connections,
                    'points_of_interest': poi
                }
            except FileNotFoundError as e:
                print(f"警告: 跳过地点 '{location_id}'，因为缺少必要的文件: {e}")
                continue
    return world_map

# --- Constants ---
CHARACTER_FILE = 'character.json'
ACTIONS_FILE = 'actions.json'
ITEMS_FILE = 'items.json'
STARTING_STORY_FILE = 'story/00_entry.md'

# --- Main Game Loop ---
def main():
    abs_path = os.path.dirname(os.path.abspath(__file__))

    # --- Session Management ---
    session_dir = os.path.join(abs_path, 'session')
    if not os.path.exists(session_dir):
        os.makedirs(session_dir)

    session_name = input("请输入会话名称 (例如 'new_game' 或 'resume_game'): ")
    session_path = os.path.join(session_dir, session_name)

    if not os.path.exists(session_path):
        os.makedirs(session_path)

    character_file_path = os.path.join(session_path, CHARACTER_FILE)

    # If no character file in session, copy from default
    if not os.path.exists(character_file_path):
        default_character_path = os.path.join(abs_path, CHARACTER_FILE)
        try:
            shutil.copy(default_character_path, character_file_path)
        except FileNotFoundError:
            print(f"错误: 找不到默认的 character.json 文件。")
            sys.exit(1)

    # --- File Paths ---
    actions_file_path = os.path.join(abs_path, ACTIONS_FILE)
    items_file_path = os.path.join(abs_path, ITEMS_FILE)
    world_root = os.path.join(abs_path, 'world')

    # --- Load Game Data ---
    try:
        character_data = load_json(character_file_path)
        actions = load_json(actions_file_path)
        items_db = load_json(items_file_path)
        world_map = load_world_map_from_folders(abs_path)
    except FileNotFoundError as e:
        print(f"错误: 无法加载游戏数据文件: {e}")
        sys.exit(1)
        
    current_story_file = os.path.join(abs_path, character_data.get('current_story_file', STARTING_STORY_FILE))

    redraw_scene = True
    while True:
        if redraw_scene:
            story_content, available_choices, metadata = parse_story_file(current_story_file, character_data)
            
            # Update location based on story file metadata
            if 'location' in metadata:
                character_data['location'] = metadata['location']

            render_scene(story_content, available_choices)

            if not available_choices:
                input("(此故事分支尚未完成，按回车键退出重试)")
                break
        
        redraw_scene = False
        prompt = f"[{character_data.get('name', 'user').lower()}@{character_data.get('pseudo_terminal_cwd', '/')}]> "
        player_input = input(prompt)

        try:
            choice_index = int(player_input) - 1
            character_data, new_story, redraw_scene = process_choice(
                choice_index, available_choices, character_data, actions, abs_path, items_db
            )
            if new_story:
                current_story_file = new_story
            
        except ValueError:
            character_data, redraw_scene = process_command(
                player_input, character_data, world_root, items_db, world_map
            )
            if not redraw_scene:
                 print()

        # Save state after every action
        character_data['current_story_file'] = os.path.relpath(current_story_file, abs_path)
        save_json(character_file_path, character_data)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n再见...")
        sys.exit(0)
