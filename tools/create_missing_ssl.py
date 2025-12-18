import os
import json
import re
import sys

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA_SCENES_DIR = os.path.join(PROJECT_ROOT, 'data', 'scenes')
STRINGS_EXTRA_DIR = os.path.join(PROJECT_ROOT, 'data', 'strings_extra')
GENERATED_STRINGS_FILE = os.path.join(STRINGS_EXTRA_DIR, 'generated_placeholders.json')
MAP_DEBUGGER_LOG = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build', 'map_debugger_output.log') # Adjusted path

def get_all_location_ids_from_debugger_log(log_path):
    location_ids = set()
    try:
        with open(log_path, 'r', encoding='utf-8') as f:
            for line in f:
                match = re.search(r'ID:\s*([a-zA-Z0-9_]+)', line)
                if match:
                    location_ids.add(match.group(1))
    except FileNotFoundError:
        print(f"Error: map_debugger output log not found at {log_path}", file=sys.stderr)
        sys.exit(1)
    return sorted(list(location_ids))

def get_existing_ssl_files():
    existing_ssl_files = set()
    for filename in os.listdir(DATA_SCENES_DIR):
        if filename.endswith('.ssl'):
            existing_ssl_files.add(filename)
    return existing_ssl_files

def create_placeholder_ssl(location_id, new_strings):
    # Standardize scene ID name
    scene_id_upper = f"SCENE_{location_id.upper()}"
    ssl_filename = f"{location_id}.ssl"
    ssl_filepath = os.path.join(DATA_SCENES_DIR, ssl_filename)
    
    if os.path.exists(ssl_filepath):
        # print(f"Skipping existing SSL file: {ssl_filepath}")
        return None # Indicate no new file created

    print(f"Creating placeholder SSL for: {location_id} (Scene ID: {scene_id_upper})")

    # Create new string IDs for this placeholder
    name_text_id = f"TEXT_SCENE_NAME_{location_id.upper()}"
    desc_text_id = f"TEXT_PLACEHOLDER_{location_id.upper()}_DESC"
    return_choice_text_id = f"TEXT_CHOICE_RETURN_FROM_{location_id.upper()}"
    
    # Add new strings to the dictionary
    new_strings[name_text_id] = location_id.replace('_', ' ').title()
    new_strings[desc_text_id] = f"你身处于{location_id.replace('_', ' ')}。这里似乎没有什么特别的。"
    new_strings[return_choice_text_id] = "离开这里" # Default return

    ssl_content = f"""
choices:
- text_id: {return_choice_text_id}
  action_id: return_from_{location_id} # Placeholder action ID
dialogue:
- speaker: SPEAKER_NONE
  text_id: {desc_text_id}
location_id: {location_id}
name_text_id: {name_text_id}
scene_id: {scene_id_upper}
"""
    with open(ssl_filepath, 'w', encoding='utf-8') as f:
        f.write(ssl_content)
    
    return scene_id_upper # Return the new scene_id created

def main():
    map_debugger_log_path = os.path.join(PROJECT_ROOT, 'map_debugger_output.log')
    # Let's read the current map_debugger output from the current temp directory
    map_debugger_log_path = '/data/data/com.termux/files/home/.gemini/tmp/d838a9463b659aa39c80da68392eba8635e883e31e874ebaf0e0578ccbf721cf/map_debugger_output.log'
    
    all_location_ids = get_all_location_ids_from_debugger_log(map_debugger_log_path)
    existing_ssl_filenames = get_existing_ssl_files()
    
    new_strings_to_add = {}
    newly_created_scenes = []

    for lid in all_location_ids:
        ssl_filename_for_loc = f"{lid}.ssl"
        scene_id_for_loc = f"SCENE_{lid.upper()}"

        # If a .ssl file for this location already exists, don't create a placeholder
        if ssl_filename_for_loc in existing_ssl_filenames:
            # print(f"SSL file for '{lid}' already exists ({ssl_filename_for_loc}). Skipping placeholder creation.")
            continue
        
        created_scene_id = create_placeholder_ssl(lid, new_strings_to_add)
        if created_scene_id:
            newly_created_scenes.append(created_scene_id)
    
    if newly_created_scenes:
        print(f"\n--- Created {len(newly_created_scenes)} placeholder SSL files ---")
        for sid in newly_created_scenes:
            print(f"- {sid}")

        print(f"\n--- Saving new string definitions to {GENERATED_STRINGS_FILE} ---")
        with open(GENERATED_STRINGS_FILE, 'w', encoding='utf-8') as f:
            json.dump(new_strings_to_add, f, indent=4, ensure_ascii=False)
        print(f"Added {len(new_strings_to_add)} new string definitions.")
    else:
        print("\nNo new placeholder SSL files needed to be created.")
    
    print("\n--- Next Steps ---")
    print(f"1. Ensure '{os.path.basename(GENERATED_STRINGS_FILE)}' is included in CMakeLists.txt for string ID generation (if not already).")
    print("2. Register newly created scenes in 'src/scenes.c'.")
    print("3. Implement actual scene content for these placeholders.")

if __name__ == "__main__":
    main()
