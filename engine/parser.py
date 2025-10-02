import json
import re

def get_nested_value(data, path):
    keys = path.split('.')
    d = data
    for key in keys:
        if isinstance(d, dict):
            d = d.get(key)
        else:
            return None
    return d

# Helper function for the new rich format
def _parse_rich_block(rules, character_data):
    flag_to_check = rules.get("flag_to_check")
    if not flag_to_check:
        return None, None

    player_value = str(get_nested_value(character_data, flag_to_check) or "")
    
    branches = rules.get("branches", {})
    # Use player_value to find the branch, fallback to "default"
    branch = branches.get(player_value) or branches.get("default")

    if not branch:
        return None, None

    text = branch.get("text", "")
    choices = branch.get("choices", [])
    return text, choices

# Helper function for the old simple format
def _parse_simple_block(rules, character_data):
    generated_fragments = []
    for state_variable, options in rules.items():
        player_value = str(character_data.get(state_variable, "0"))
        if player_value in options:
            generated_fragments.append(options[player_value])
        elif "default" in options:
            generated_fragments.append(options["default"])
    
    text = " ".join(filter(None, generated_fragments))
    return text, None # Simple format doesn't have choices

def parse_story_file(file_path, character_data):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except FileNotFoundError:
        return [f"错误：找不到故事文件 {file_path}"], [], {}

    story_content = []
    choices = []
    metadata = {}
    iterator = iter(lines)

    # Handle Front Matter
    try:
        if next(iterator).strip() == '---':
            for meta_line in iterator:
                if meta_line.strip() == '---':
                    break
                parts = meta_line.strip().split(':', 1)
                if len(parts) == 2:
                    metadata[parts[0].strip()] = parts[1].strip()
        else:
            # Reset iterator if no front matter
            iterator = iter(lines)
    except StopIteration:
        iterator = iter(lines) # Reset for empty files

    choice_pattern = re.compile(r'-\s*\[([^\]]*)\]\(action:(.*?)\)')
    pause_pattern = re.compile(r'^\[PAUSE:([\d.]+)\]$')

    text_buffer = []

    def flush_text_buffer():
        nonlocal text_buffer
        if text_buffer:
            story_content.append("".join(text_buffer))
            text_buffer = []

    for line in iterator:
        stripped_line = line.strip()

        if stripped_line.startswith('<!--'):
            continue

        # Handle Combinatorial Block
        if stripped_line == '{':
            flush_text_buffer()
            block_buffer = line
            
            while True:
                try:
                    next_line = next(iterator)
                    block_buffer += next_line
                    
                    try:
                        rules = json.loads(block_buffer)
                        
                        # DECIDE WHICH PARSER TO USE
                        if "branches" in rules:
                            text, new_choices = _parse_rich_block(rules, character_data)
                        else:
                            text, new_choices = _parse_simple_block(rules, character_data)

                        if text:
                            story_content.append(text)
                        if new_choices:
                            choices.extend(new_choices)
                        
                        break # Exit the while loop

                    except json.JSONDecodeError:
                        # This is expected, means the object is not yet complete
                        continue

                except StopIteration:
                    # Reached end of file while in a block, append as raw
                    story_content.append(block_buffer)
                    break
            continue

        # Handle Choices (for non-block choices)
        choice_match = choice_pattern.match(stripped_line)
        if choice_match:
            flush_text_buffer()
            choices.append({"text": choice_match.group(1).strip(), "action": choice_match.group(2).strip()})
            continue

        # Handle Pauses
        pause_match = pause_pattern.fullmatch(stripped_line)
        if pause_match:
            flush_text_buffer()
            try:
                story_content.append(("pause", float(pause_match.group(1))))
            except (ValueError, IndexError):
                text_buffer.append(line)
            continue
            
        # Buffer normal text
        text_buffer.append(line)

    flush_text_buffer()
    story_content = [s for s in story_content if s.strip()]
    return story_content, choices, metadata