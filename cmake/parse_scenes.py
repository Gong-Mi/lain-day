import yaml
import sys
import os
import re

# Assume string_ids.h and speaker_ids.h will be generated or available
# For now, we'll hardcode some valid IDs for validation purposes.
VALID_STRING_IDS = set() # This will be populated from generated string_ids.h
VALID_SPEAKER_IDS = {
    "SPEAKER_NONE", "SPEAKER_LAIN", "SPEAKER_MOM", "SPEAKER_DAD", 
    "SPEAKER_ALICE", "SPEAKER_CHISA", "SPEAKER_MIKA", "SPEAKER_GHOST", 
    "SPEAKER_DOCTOR", "SPEAKER_NAVI", "SPEAKER_PARENT"
}
VALID_ACTION_IDS = set() # This will need to be dynamically loaded or hardcoded
VALID_LOCATION_IDS = set() # This will need to be dynamically loaded or hardcoded


def load_valid_string_ids(string_ids_header_path):
    """Loads valid string IDs from the generated string_ids.h file."""
    if not os.path.exists(string_ids_header_path):
        print(f"Error: string_ids.h not found at {string_ids_header_path}. Ensure it's generated.", file=sys.stderr)
        sys.exit(1)
    
    with open(string_ids_header_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Regex to find enum members, excluding TEXT_COUNT, TEXT_INVALID, TEXT_EMPTY_LINE
    # Matches any uppercase identifier followed by optional assignment and optional comma
    matches = re.findall(r'^\s*([A-Z][A-Z0-9_]+)(?:\s*=\s*\d+)?\s*,?', content, re.MULTILINE)
    
    # Filter out TEXT_COUNT, TEXT_INVALID, TEXT_EMPTY_LINE as they are special
    for match in matches:
        if match not in ["TEXT_COUNT", "TEXT_INVALID"]:
            VALID_STRING_IDS.add(match.strip())
    
    print(f"DEBUG: Loaded IDs: {sorted(list(VALID_STRING_IDS))}")
    print(f"Loaded {len(VALID_STRING_IDS)} valid string IDs.")


def parse_and_validate_ssl(ssl_path, generated_c_header_path, generated_c_source_path):
    # Ensure output directories exist
    os.makedirs(os.path.dirname(generated_c_header_path), exist_ok=True)
    os.makedirs(os.path.dirname(generated_c_source_path), exist_ok=True)

    try:
        with open(ssl_path, 'r', encoding='utf-8') as f:
            scene_data = yaml.safe_load(f)
    except FileNotFoundError:
        print(f"Error: SSL file not found at {ssl_path}", file=sys.stderr)
        sys.exit(1)
    except yaml.YAMLError as e:
        print(f"Error: Malformed YAML in {ssl_path}: {e}", file=sys.stderr)
        sys.exit(1)

    scene_id = scene_data.get('scene_id')
    if not scene_id or not isinstance(scene_id, str):
        print(f"Error: '{ssl_path}' is missing or has invalid 'scene_id'.", file=sys.stderr)
        sys.exit(1)

    # Basic validation (extend as needed)
    if 'name_text_id' not in scene_data or scene_data['name_text_id'] not in VALID_STRING_IDS:
        print(f"Error: '{ssl_path}' has invalid or missing 'name_text_id'. Must be a valid StringID.", file=sys.stderr)
        sys.exit(1)

    if 'location_id' not in scene_data or not isinstance(scene_data['location_id'], str):
        print(f"Error: '{ssl_path}' has invalid or missing 'location_id'.", file=sys.stderr)
        sys.exit(1)
    # TODO: Validate location_id against known valid location IDs

    # Dialogue validation
    if 'dialogue' in scene_data:
        for i, line in enumerate(scene_data['dialogue']):
            if 'speaker' not in line or line['speaker'] not in VALID_SPEAKER_IDS:
                print(f"Error: '{ssl_path}' dialogue line {i} has invalid or missing 'speaker'.", file=sys.stderr)
                sys.exit(1)
            if 'text_id' not in line or line['text_id'] not in VALID_STRING_IDS:
                print(f"Error: '{ssl_path}' dialogue line {i} has invalid or missing 'text_id'.", file=sys.stderr)
                sys.exit(1)

    # Choices validation
    if 'choices' in scene_data:
        for i, choice in enumerate(scene_data['choices']):
            if 'text_id' not in choice or choice['text_id'] not in VALID_STRING_IDS:
                print(f"Error: '{ssl_path}' choice {i} has invalid or missing 'text_id'.", file=sys.stderr)
                sys.exit(1)
            if 'action_id' not in choice or not isinstance(choice['action_id'], str):
                print(f"Error: '{ssl_path}' choice {i} has invalid or missing 'action_id'.", file=sys.stderr)
                sys.exit(1)
            # TODO: Validate action_id against known valid action IDs
            if 'target_scene' in choice and not isinstance(choice['target_scene'], str):
                 print(f"Error: '{ssl_path}' choice {i} has invalid 'target_scene'.", file=sys.stderr)
                 sys.exit(1)
            # TODO: Validate target_scene against known valid scene_ids

            if 'condition' in choice:
                condition = choice['condition']
                if 'flag' not in condition or not isinstance(condition['flag'], str):
                    print(f"Error: '{ssl_path}' choice {i} condition has invalid or missing 'flag'.", file=sys.stderr)
                    sys.exit(1)
                if 'value' not in condition or not isinstance(condition['value'], int):
                    print(f"Error: '{ssl_path}' choice {i} condition has invalid or missing 'value'.", file=sys.stderr)
                    sys.exit(1)
                # TODO: Validate flag against known valid flags

    # Generate a simple C header and source for now, to be expanded
    with open(generated_c_header_path, 'w', encoding='utf-8') as f:
        f.write("#ifndef SCENE_DATA_{}_H\n".format(scene_id.upper()))
        f.write("#define SCENE_DATA_{}_H\n\n".format(scene_id.upper()))
        f.write("#include \"game_types.h\"\n\n")
        f.write("// Declaration for scene {}\n".format(scene_id))
        f.write("extern StoryScene g_{}_scene_data;\n\n".format(scene_id.lower()))
        f.write("void init_scene_{}_from_data(StoryScene* scene);\n\n".format(scene_id.lower()))
        f.write("#endif // SCENE_DATA_{}_H\n".format(scene_id.upper()))
    
    print(f"Generated {generated_c_header_path} for scene {scene_id}.")

    with open(generated_c_source_path, 'w', encoding='utf-8') as f:
        f.write("#include \"{}\"\n".format(os.path.basename(generated_c_header_path)))
        f.write("#include \"string_ids.h\"\n")
        f.write("#include \"scenes.h\" // For SPEAKER_... enums if directly used\n") 
        f.write("#include \"string_table.h\" // For get_string_by_id\n")
        f.write("#include <string.h>\n\n")

        # Static StoryScene definition
        f.write("// Definition for scene {}\n".format(scene_id))
        f.write("StoryScene g_{}_scene_data = {{\n".format(scene_id.lower()))
        f.write("    .scene_id = \"{}\",\n".format(scene_id))
        # For name_text_id, we just store the StringID, not the resolved text
        f.write("    .name = \"\", // Resolved at runtime via get_string_by_id(name_text_id)\n")
        f.write("    .location_id = \"{}\",\n".format(scene_data.get('location_id', '')))
        f.write("    .dialogue_line_count = 0,\n")
        f.write("    .choice_count = 0,\n")
        f.write("};\n\n")

        # init_scene_..._from_data function
        f.write("void init_scene_{}_from_data(StoryScene* scene) {{\n".format(scene_id.lower()))
        f.write("    // Use the g_{}_scene_data as a template\n".format(scene_id.lower()))
        f.write("    memcpy(scene, &g_{}_scene_data, sizeof(StoryScene));\n".format(scene_id.lower()))
        
        # For dialogue
        if 'dialogue' in scene_data and scene_data['dialogue']:
            f.write("    scene->dialogue_line_count = {};\n".format(len(scene_data['dialogue'])))
            for i, line in enumerate(scene_data['dialogue']):
                speaker_id = line['speaker']
                text_id = line['text_id']
                f.write("    scene->dialogue_lines[{}] = (DialogueLine){{ {}, {} }};\n".format(i, speaker_id, text_id))
        
        # For choices
        if 'choices' in scene_data and scene_data['choices']:
            f.write("    scene->choice_count = {};\n".format(len(scene_data['choices'])))
            for i, choice in enumerate(scene_data['choices']):
                text_id = choice['text_id']
                action_id = choice['action_id']
                
                # Basic choice initialization
                f.write("    scene->choices[{}] = (StoryChoice){{ .text_id = {}, .action_id = \"{}\", .condition_count = 0 }};\n".format(i, text_id, action_id))
                
                # New conditions block
                if 'conditions' in choice and isinstance(choice['conditions'], list):
                    conditions = choice['conditions']
                    f.write("    scene->choices[{}].condition_count = {};\n".format(i, len(conditions)))
                    for cond_idx, condition in enumerate(conditions):
                        flag_name = condition.get('requires_flag', '')
                        flag_value = condition.get('flag_value', '')
                        min_day = condition.get('min_day', -1)
                        max_day = condition.get('max_day', -1)
                        exact_day = condition.get('exact_day', -1)
                        
                        hour_is_between = condition.get('hour_is_between', [-1, -1])
                        hour_start = hour_is_between[0] if isinstance(hour_is_between, list) and len(hour_is_between) == 2 else -1
                        hour_end = hour_is_between[1] if isinstance(hour_is_between, list) and len(hour_is_between) == 2 else -1

                        f.write("    scene->choices[{}].conditions[{}] = (Condition){{\n".format(i, cond_idx))
                        f.write("        .flag_name = \"{}\",\n".format(flag_name))
                        f.write("        .required_value = \"{}\",\n".format(flag_value))
                        f.write("        .min_day = {},\n".format(min_day))
                        f.write("        .max_day = {},\n".format(max_day))
                        f.write("        .exact_day = {},\n".format(exact_day))
                        f.write("        .hour_start = {},\n".format(hour_start))
                        f.write("        .hour_end = {},\n".format(hour_end))
                        f.write("    };\n")

        # For auto_events
        if 'auto_events' in scene_data and scene_data['auto_events']:
            f.write("    scene->auto_event_count = {};\n".format(len(scene_data['auto_events'])))
            for i, event in enumerate(scene_data['auto_events']):
                target_scene = event.get('target_scene', '')
                wait_time = event.get('wait_time', 0)
                flag_set = event.get('flag_set', '')
                
                f.write("    strncpy(scene->auto_events[{}].target_scene_id, \"{}\", MAX_NAME_LENGTH - 1);\n".format(i, target_scene))
                f.write("    scene->auto_events[{}].wait_time = {};\n".format(i, wait_time))
                f.write("    strncpy(scene->auto_events[{}].flag_to_set, \"{}\", MAX_NAME_LENGTH - 1);\n".format(i, flag_set))

                if 'conditions' in event and isinstance(event['conditions'], list):
                    conditions = event['conditions']
                    f.write("    scene->auto_events[{}].condition_count = {};\n".format(i, len(conditions)))
                    for cond_idx, condition in enumerate(conditions):
                        flag_name = condition.get('requires_flag', '')
                        flag_value = condition.get('flag_value', '')
                        min_day = condition.get('min_day', -1)
                        max_day = condition.get('max_day', -1)
                        exact_day = condition.get('exact_day', -1)
                        
                        hour_is_between = condition.get('hour_is_between', [-1, -1])
                        hour_start = hour_is_between[0] if isinstance(hour_is_between, list) and len(hour_is_between) == 2 else -1
                        hour_end = hour_is_between[1] if isinstance(hour_is_between, list) and len(hour_is_between) == 2 else -1

                        f.write("    scene->auto_events[{}].conditions[{}] = (Condition){{\n".format(i, cond_idx))
                        f.write("        .flag_name = \"{}\",\n".format(flag_name))
                        f.write("        .required_value = \"{}\",\n".format(flag_value))
                        f.write("        .min_day = {},\n".format(min_day))
                        f.write("        .max_day = {},\n".format(max_day))
                        f.write("        .exact_day = {},\n".format(exact_day))
                        f.write("        .hour_start = {},\n".format(hour_start))
                        f.write("        .hour_end = {},\n".format(hour_end))
                        f.write("    };\n")


        # Resolve scene name using StringID
        f.write("    strcpy(scene->name, get_string_by_id({}));\n".format(scene_data['name_text_id']))

        f.write("}\n")
    print(f"Generated {generated_c_source_path} for scene {scene_id}.")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python parse_scenes.py <path_to_ssl_file> <path_to_string_ids_h> <path_to_generated_scene_header> <path_to_generated_scene_source>", file=sys.stderr)
        sys.exit(1)

    ssl_input_path = sys.argv[1]
    string_ids_header_path = sys.argv[2]
    generated_c_header_path = sys.argv[3]
    generated_c_source_path = sys.argv[4]

    load_valid_string_ids(string_ids_header_path)
    # TODO: Load VALID_SPEAKER_IDS, VALID_ACTION_IDS, VALID_LOCATION_IDS dynamically
    
    parse_and_validate_ssl(ssl_input_path, generated_c_header_path, generated_c_source_path)
