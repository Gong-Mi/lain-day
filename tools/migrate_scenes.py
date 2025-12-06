import os
import re
import yaml

def migrate_scene_c_to_ssl(c_file_path, output_dir):
    """
    Parses a C scene file line-by-line and converts it to the SSL YAML format.
    """
    with open(c_file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    in_init_func = False
    func_body_lines = []
    scene_func_name = ""

    for line in lines:
        if line.strip().startswith("void init_scene_"):
            in_init_func = True
            match = re.search(r'void\s+init_scene_([a-zA-Z0-9_]+)', line)
            if match:
                scene_func_name = match.group(1)
        elif in_init_func:
            if line.strip() == "}":
                in_init_func = False
            else:
                func_body_lines.append(line)

    if not scene_func_name:
        print(f"Warning: Could not find init function in {c_file_path}. Skipping.")
        return

    func_body = "".join(func_body_lines)
    
    scene_id_match = re.search(r'strcpy\s*\(\s*scene->scene_id,\s*"(.*?)"\s*\);', func_body)
    scene_id = scene_id_match.group(1) if scene_id_match else f"SCENE_{scene_func_name.upper()}"
    
    name_match = re.search(r'strcpy\s*\(\s*scene->name,\s*"(.*?)"\s*\);', func_body)
    name = name_match.group(1) if name_match else ""

    location_id_match = re.search(r'strcpy\s*\(\s*scene->location_id,\s*"(.*?)"\s*\);', func_body)
    location_id = location_id_match.group(1) if location_id_match else ""
    
    name_text_id = f"TEXT_SCENE_NAME_{scene_func_name.upper()}"

    dialogue_lines = []
    dialogue_matches = re.findall(r'scene->dialogue_lines\[\d+\].*?=\s*\(DialogueLine\)\s*\{\s*\.speaker_id\s*=\s*(.*?)\s*,\s*\.text_id\s*=\s*(.*?)\s*\}\s*;', func_body)
    for speaker, text_id in dialogue_matches:
        dialogue_lines.append({'speaker': speaker.strip(), 'text_id': text_id.strip()})

    choices = []
    choice_matches = re.findall(r'scene->choices\[\d+\].*?=\s*\(StoryChoice\)\s*\{\s*\.text_id\s*=\s*(.*?)\s*,\s*\.action_id\s*=\s*"(.*?)"\s*(,.*?)?\}\s*;', func_body)
    for text_id, action_id, condition_str in choice_matches:
        choice_data = {'text_id': text_id.strip(), 'action_id': action_id}
        if condition_str and condition_str.strip():
             condition_data = {}
             flag_match = re.search(r'"(.*?)"', condition_str)
             value_match = re.search(r',\s*(\d+)', condition_str)
             if flag_match:
                 condition_data['flag'] = flag_match.group(1)
             if value_match:
                 condition_data['value'] = int(value_match.group(1))
             if condition_data:
                choice_data['condition'] = condition_data
        choices.append(choice_data)

    ssl_data = {
        'scene_id': scene_id,
        'name_text_id': name_text_id,
        'location_id': location_id,
        'dialogue': dialogue_lines,
        'choices': choices
    }
    
    output_filename = os.path.join(output_dir, f"{scene_func_name}.ssl")
    with open(output_filename, 'w', encoding='utf-8') as f:
        yaml.dump(ssl_data, f, default_flow_style=False, allow_unicode=True)

    print(f"Migrated {c_file_path} to {output_filename}")


if __name__ == "__main__":
    scenes_dir = "scenes"
    output_dir = "data/scenes"
    
    os.makedirs(output_dir, exist_ok=True)
    
    for root, dirs, files in os.walk(scenes_dir):
        for file in files:
            if file == "scene.c":
                c_file_path = os.path.join(root, file)
                migrate_scene_c_to_ssl(c_file_path, output_dir)
                
    print("Migration complete.")