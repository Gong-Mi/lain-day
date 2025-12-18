#!/usr/bin/env python3
import os
import json
import re
import glob

# Configuration
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STRINGS_DIR = os.path.join(PROJECT_ROOT, 'data', 'strings_extra')
MAIN_STRINGS_FILE = os.path.join(PROJECT_ROOT, 'data', 'strings.json')
SCENES_DIR = os.path.join(PROJECT_ROOT, 'data', 'scenes')
SRC_DIRS = [
    os.path.join(PROJECT_ROOT, 'src'),
    os.path.join(PROJECT_ROOT, 'sequences')
]

# Regex patterns
STRING_ID_REGEX = re.compile(r'get_string_by_id\s*\(\s*([A-Z0-9_]+)\s*\)')
SCENE_ID_REGEX_C = re.compile(r'"(SCENE_[A-Z0-9_]+)"') # Matches "SCENE_..." in C
TEXT_ID_REGEX_SSL = re.compile(r'text_id:\s*([A-Z0-9_]+)')

# Data containers
defined_string_ids = set()
defined_scene_ids = set()
referenced_string_ids = set()
referenced_scene_ids = set()

def load_string_definitions():
    print("--- Loading String Definitions ---")
    files = glob.glob(os.path.join(STRINGS_DIR, '*.json'))
    if os.path.exists(MAIN_STRINGS_FILE):
        files.append(MAIN_STRINGS_FILE)
    
    for filepath in files:
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                data = json.load(f)
                for key in data.keys():
                    defined_string_ids.add(key)
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
    print(f"Found {len(defined_string_ids)} string definitions.")

def load_scene_definitions():
    print("--- Loading Scene Definitions ---")
    files = glob.glob(os.path.join(SCENES_DIR, '*.ssl'))
    for filepath in files:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
            # Basic parsing to find scene_id
            match = re.search(r'scene_id:\s*([A-Z0-9_]+)', content)
            if match:
                defined_scene_ids.add(match.group(1))
            
            # While we are here, find text references in SSL
            text_matches = TEXT_ID_REGEX_SSL.findall(content)
            for tid in text_matches:
                referenced_string_ids.add(tid)
                
    print(f"Found {len(defined_scene_ids)} scene definitions.")

def scan_c_code():
    print("--- Scanning C Code for References ---")
    for src_dir in SRC_DIRS:
        for root, dirs, files in os.walk(src_dir):
            for file in files:
                if file.endswith('.c') or file.endswith('.h'):
                    filepath = os.path.join(root, file)
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        
                        # Find String ID references
                        s_matches = STRING_ID_REGEX.findall(content)
                        for sid in s_matches:
                            referenced_string_ids.add(sid)
                            
                        # Find Scene ID references
                        sc_matches = SCENE_ID_REGEX_C.findall(content)
                        for scid in sc_matches:
                            referenced_scene_ids.add(scid)

def report():
    print("\n=== CONSISTENCY CHECK REPORT ===\n")
    
    # Check 1: Undefined Strings
    print("[ Strings Check ]")
    undefined_strings = referenced_string_ids - defined_string_ids
    # Filter out some known dynamic or system IDs if necessary
    undefined_strings = {s for s in undefined_strings if s != "TEXT_INVALID"}
    
    if undefined_strings:
        print(f"CRITICAL: Found {len(undefined_strings)} undefined string IDs referenced in code/data:")
        for s in sorted(undefined_strings):
            print(f"  - {s}")
    else:
        print("OK: All referenced strings are defined.")

    print("\n[ Scenes Check ]")
    # Check 2: Undefined Scenes
    # Filter out potential generic placeholders or dynamic ones if not in SSL
    # e.g. SCENE_SIDE_STORIES_... might not have SSLs yet
    undefined_scenes = referenced_scene_ids - defined_scene_ids
    
    if undefined_scenes:
        print(f"WARNING: Found {len(undefined_scenes)} referenced scenes with no matching .ssl file:")
        for s in sorted(undefined_scenes):
            print(f"  - {s}")
    else:
        print("OK: All referenced scenes have definitions.")

if __name__ == "__main__":
    load_string_definitions()
    load_scene_definitions()
    scan_c_code()
    report()
