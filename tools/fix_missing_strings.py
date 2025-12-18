import json
import glob
import os
import re

def load_json(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return json.load(f)

def save_json(filepath, data):
    with open(filepath, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=4, ensure_ascii=False)

def main():
    strings_map_path = 'data/strings_extra/strings_map.json'
    strings_data = load_json(strings_map_path)
    
    ssl_files = glob.glob('data/scenes/*.ssl')
    
    modified = False
    
    for ssl_file in ssl_files:
        with open(ssl_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Regex to find name_text_id: ...
        match = re.search(r'name_text_id:\s*(\w+)', content)
        if match:
            string_id = match.group(1)
            if string_id not in strings_data:
                print(f"Adding missing ID: {string_id} from {ssl_file}")
                strings_data[string_id] = f"Location: {string_id}" # Placeholder
                modified = True

        # Regex to find text_id in choices or dialogue if clearly missing
        # This is harder without a full parser, but let's check for specific patterns
        # if we see them failing. For now, name_text_id is the main build blocker.
        
        # Check for dialogue text_id placeholders
        placeholders = re.findall(r'text_id:\s*(TEXT_PLACEHOLDER_\w+)', content)
        for ph in placeholders:
            if ph not in strings_data:
                print(f"Adding missing placeholder: {ph} from {ssl_file}")
                strings_data[ph] = "Description placeholder."
                modified = True

        # Check for choice text_id placeholders
        choice_placeholders = re.findall(r'text_id:\s*(TEXT_CHOICE_\w+)', content)
        for ph in choice_placeholders:
             if ph not in strings_data:
                print(f"Adding missing choice placeholder: {ph} from {ssl_file}")
                strings_data[ph] = "Action placeholder"
                modified = True

    if modified:
        save_json(strings_map_path, strings_data)
        print("Updated strings_map.json")
    else:
        print("No changes needed.")

if __name__ == "__main__":
    main()
