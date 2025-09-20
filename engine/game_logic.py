import os
import time
from .renderer import typewriter_print

def process_choice(choice_index, available_choices, character_data, actions, abs_path):
    """
    Processes the player's validated numeric choice.

    Returns a tuple: (new_character_data, new_story_file, needs_redraw)
    """
    if 0 <= choice_index < len(available_choices):
        chosen_choice = available_choices[choice_index]
        if "(无法选择)" in chosen_choice['text']:
            input("这个选项当前无法选择。按回车键重试。")
            return character_data, None, True

        chosen_action_name = chosen_choice['action']
        if chosen_action_name in actions:
            action_details = actions[chosen_action_name]
            payload = action_details.get('payload', {})
            action_type = action_details.get('type')

            new_story_file = None

            if action_type == 'location_change':
                character_data['location'] = payload.get('new_location')
                new_story_file = os.path.join(abs_path, payload.get('story_file'))
            elif action_type == 'location_change_and_set_flags':
                if 'flags' in payload:
                    for flag in payload['flags']:
                        character_data[flag['name']] = flag['value']
                character_data['location'] = payload.get('new_location')
                new_story_file = os.path.join(abs_path, payload.get('story_file'))
            elif action_type == 'unlock_command':
                cmd_to_unlock = payload.get('command')
                if cmd_to_unlock and cmd_to_unlock not in character_data['unlocked_commands']:
                    character_data['unlocked_commands'].append(cmd_to_unlock)
                    typewriter_print(f"\n[新指令已解锁: {cmd_to_unlock}]\n")
                    time.sleep(1.5)
                if 'story_file' in payload:
                     new_story_file = os.path.join(abs_path, payload.get('story_file'))

            elif action_type == 'story_change_and_set_flags':
                if 'flags' in payload:
                    for flag in payload['flags']:
                        character_data[flag['name']] = flag['value']
                new_story_file = os.path.join(abs_path, payload.get('story_file'))
            else: # Default to simple story change
                new_story_file = os.path.join(abs_path, payload.get('story_file'))

            return character_data, new_story_file, True
        else:
            input(f"错误: 动作 '{chosen_action_name}' 未定义。按回车键。")
            return character_data, None, True
    else:
        input("无效的选择。按回车键重试。")
        return character_data, None, True
