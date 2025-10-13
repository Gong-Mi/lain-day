import os
import time
from .renderer import typewriter_print

def set_nested_value(data, path, value):
    keys = path.split('.')
    d = data
    for i, key in enumerate(keys):
        if i == len(keys) - 1:
            d[key] = value
        else:
            if key not in d or not isinstance(d[key], dict):
                d[key] = {}
            d = d[key]

def execute_action(action_details, character_data, abs_path, items_db, action_name=None):
    """
    Executes a given action and updates character_data.

    Returns a tuple: (new_character_data, new_story_file, needs_redraw, action_name)
    """
    payload = action_details.get('payload', {})
    action_type = action_details.get('type')
    new_story_file = None
    needs_redraw = True

    if action_type == 'story_change':
        new_story_file = os.path.join(abs_path, payload.get('story_file'))

    elif action_type == 'enter_story':
        character_data['return_to_story_file'] = character_data['current_story_file']
        new_story_file = os.path.join(abs_path, payload.get('story_file'))

    elif action_type == 'exit_story':
        return_file = character_data.get('return_to_story_file')
        if return_file:
            new_story_file = os.path.join(abs_path, return_file)
            character_data['return_to_story_file'] = None
        else:
            typewriter_print("\n[错误: 没有可返回的故事点。]\n")
            time.sleep(1.5)
            new_story_file = os.path.join(abs_path, character_data['current_story_file'])

    elif action_type == 'conditional_story_change':
        flag_name = payload.get('flag_name')
        story_if_true = payload.get('story_if_true')
        story_if_false = payload.get('story_if_false')
        if character_data.get(flag_name):
            new_story_file = os.path.join(abs_path, story_if_true)
        else:
            new_story_file = os.path.join(abs_path, story_if_false)

    elif action_type == 'location_change':
        character_data['location'] = payload.get('new_location')
        new_story_file = os.path.join(abs_path, payload.get('story_file'))

    elif action_type in ['location_change_and_set_flags', 'story_change_and_set_flags']:
        if 'flags' in payload:
            for flag in payload['flags']:
                set_nested_value(character_data, flag['name'], flag['value'])
        if action_type == 'location_change_and_set_flags':
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

    elif action_type == 'story_change_and_unlock_set':
        commands_to_unlock = payload.get('commands', [])
        newly_unlocked = []
        if commands_to_unlock:
            for cmd in commands_to_unlock:
                if cmd not in character_data['unlocked_commands']:
                    character_data['unlocked_commands'].append(cmd)
                    newly_unlocked.append(cmd)
        if newly_unlocked:
            typewriter_print(f"\n[基础指令系统已解锁: {', '.join(newly_unlocked)}]\n")
            time.sleep(1.5)
        new_story_file = os.path.join(abs_path, payload.get('story_file'))

    elif action_type == 'acquire_item':
        item_id = payload.get('item_id')
        item_data = items_db.get(item_id)
        if not item_data:
            typewriter_print(f"\n[错误: 物品 '{item_id}' 未在 items.json 中定义。]\n")
            time.sleep(1.5)
            return character_data, None, True, action_name
        player_credit = character_data.get('credit_level', 0)
        required_credit = item_data.get('required_credit', 1)
        if player_credit >= required_credit:
            inventory = character_data.get('inventory', {})
            inventory[item_id] = inventory.get(item_id, 0) + 1
            character_data['inventory'] = inventory
            typewriter_print(f"\n[获得物品: {item_data.get('name', item_id)}]\n")
            time.sleep(1.5)
        else:
            typewriter_print(f"\n[权限不足: 需要信用等级 {required_credit}]\n")
            time.sleep(1.5)
        if payload.get('story_file'):
            new_story_file = os.path.join(abs_path, payload.get('story_file'))
        return character_data, new_story_file, True, action_name

    elif action_type == 'acquire_item_and_set_flag':
        # 1. Acquire Item
        item_id = payload.get('item_id')
        item_data = items_db.get(item_id)
        if not item_data:
            typewriter_print(f"\n[错误: 物品 '{item_id}' 未在 items.json 中定义。]\n")
            time.sleep(1.5)
            return character_data, None, True, action_name
        
        player_credit = character_data.get('credit_level', 0)
        required_credit = item_data.get('required_credit', 0)

        if player_credit >= required_credit:
            inventory = character_data.get('inventory', {})
            if item_id not in inventory:
                inventory[item_id] = 0
            inventory[item_id] += 1
            character_data['inventory'] = inventory
            typewriter_print(f"\n[获得物品: {item_data.get('name', item_id)}]\n")
            time.sleep(1.5)

            # 2. Set Flag
            if 'flag' in payload:
                flag = payload['flag']
                set_nested_value(character_data, flag['name'], flag['value'])
        else:
            typewriter_print(f"\n[权限不足: 需要信用等级 {required_credit}]\n")
            time.sleep(1.5)

        # 3. Redraw scene
        if payload.get('story_file'):
            new_story_file = os.path.join(abs_path, payload.get('story_file'))
        return character_data, new_story_file, True, action_name

    elif action_type == 'toggle_protocol':
        protocol_name = payload.get('protocol_name')
        protocols = character_data.get('network_status', {}).get('protocols', {})
        
        if protocol_name in protocols:
            current_status = protocols[protocol_name]
            if current_status == 'on':
                # Check if it's the last active protocol
                active_count = sum(1 for status in protocols.values() if status == 'on')
                if active_count <= 1:
                    typewriter_print("\n[至少需要一个激活的协议。]\n")
                    time.sleep(1.5)
                else:
                    protocols[protocol_name] = 'off'
            else:
                protocols[protocol_name] = 'on'
        
        # Reload the current scene to show the change
        new_story_file = os.path.join(abs_path, character_data['current_story_file'])

    else:
        typewriter_print(f"\n[警告: 未知的动作类型 '{action_type}']\n")
        time.sleep(1.5)
        needs_redraw = False

    return character_data, new_story_file, needs_redraw, action_name

def process_choice(choice_index, available_choices, character_data, actions, abs_path, items_db):
    """
    Processes the player's validated numeric choice.
    Finds the action and passes it to execute_action.

    Returns a tuple: (new_character_data, new_story_file, needs_redraw)
    """
    if not (0 <= choice_index < len(available_choices)):
        input("无效的选择。按回车键重试。")
        return character_data, None, True, None

    chosen_choice = available_choices[choice_index]
    if "(无法选择)" in chosen_choice['text']:
        input("这个选项当前无法选择。按回车键重试。")
        return character_data, None, True, None

    chosen_action_name = chosen_choice['action']
    if chosen_action_name not in actions:
        input(f"错误: 动作 '{chosen_action_name}' 未定义。按回车键。")
        return character_data, None, True, None

    action_details = actions[chosen_action_name]

    # Call the new centralized function
    return execute_action(action_details, character_data, abs_path, items_db, action_name=chosen_action_name)
