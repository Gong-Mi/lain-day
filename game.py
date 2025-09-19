
import json
import os
import re
import sys
import time
import shlex

# --- Constants ---
CHARACTER_FILE = 'character.json'
ACTIONS_FILE = 'actions.json'
ITEMS_FILE = 'items.json' # New
STARTING_STORY_FILE = 'story/00_entry.md'

# --- Core Helpers ---
def load_json(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def save_json(file_path, data):
    with open(file_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

def typewriter_print(text, delay=0.04):
    for char in text:
        sys.stdout.write(char)
        sys.stdout.flush()
        time.sleep(delay)

# --- Story Parser ---
def parse_story_file(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except FileNotFoundError:
        return f"错误：找不到故事文件 {file_path}", []
    text_content = []
    choices = []
    choice_pattern = re.compile(r'-\s*\[(.*?)\]\(action:(.*?)\)')
    for line in content.splitlines():
        match = choice_pattern.match(line.strip())
        if match:
            choices.append({"text": match.group(1).strip(), "action": match.group(2).strip()})
        else:
            text_content.append(line)
    return "\n".join(text_content), choices

# --- Terminal Command Handlers ---
def get_jailed_path(world_root, sim_cwd, target_path):
    if not target_path:
        target_path = '.'
    if target_path.startswith('/'):
        sim_full_path = target_path
    else:
        sim_full_path = os.path.join(sim_cwd, target_path)

    norm_sim_path = os.path.normpath(sim_full_path)
    if norm_sim_path.startswith(".."):
        return None, None

    real_path = os.path.join(world_root, norm_sim_path.lstrip('/'))
    
    if os.path.commonpath([world_root, real_path]) != world_root:
        return None, None

    return real_path, norm_sim_path

def handle_help(character_data):
    print("可用指令:")
    unlocked = sorted(character_data.get("unlocked_commands", []))
    if "help" in unlocked: print("  help              - 显示此帮助信息")
    if "inventory" in unlocked: print("  inventory/inv     - 显示物品栏")
    if "ls" in unlocked: print("  ls [路径]         - 列出文件和目录")
    if "cd" in unlocked: print("  cd <路径>         - 切换目录")
    if "cat" in unlocked: print("  cat <文件名>      - 查看文件内容")
    if "whoami" in unlocked: print("  whoami            - 显示当前用户")
    if "clear" in unlocked or "cls" in unlocked: print("  clear/cls         - 清空屏幕")
    if "cmake" in unlocked: print("  cmake .           - (模拟) 尝试构建当前目录的项目")

def handle_inventory(character_data, items_db):
    inventory_ids = character_data.get("inventory", [])
    if not inventory_ids:
        print("你的物品栏是空的。")
        return
    
    print("物品栏:")
    for item_id in inventory_ids:
        item = items_db.get(item_id)
        if item:
            print(f"  - {item.get('name', item_id)}: {item.get('description', '')}")
        else:
            print(f"  - {item_id} (未知物品)")

def handle_ls(args, world_root, sim_cwd):
    target_sim_path = args[0] if args else sim_cwd
    real_path, _ = get_jailed_path(world_root, sim_cwd, target_sim_path)

    if not real_path or not os.path.isdir(real_path):
        print(f"ls: cannot access '{target_sim_path}': No such file or directory")
        return
    
    for item in sorted(os.listdir(real_path)):
        print(item)

def handle_cd(args, world_root, sim_cwd):
    if not args:
        return sim_cwd

    target_dir = args[0]
    real_path, new_sim_path = get_jailed_path(world_root, sim_cwd, target_dir)

    if not real_path or not os.path.isdir(real_path):
        print(f"cd: no such file or directory: {target_dir}")
        return sim_cwd
    
    return new_sim_path

def handle_cat(args, world_root, sim_cwd):
    if not args:
        print("cat: missing operand")
        return

    real_path, _ = get_jailed_path(world_root, sim_cwd, args[0])
    if not real_path or not os.path.isfile(real_path):
        print(f"cat: {args[0]}: No such file or directory")
        return
    
    with open(real_path, 'r', encoding='utf-8') as f:
        print(f.read())

def handle_cmake(args, world_root, sim_cwd):
    if not args or args[0] != '.':
        print("Usage: cmake .")
        return

    cmakelists_path, _ = get_jailed_path(world_root, sim_cwd, 'CMakeLists.txt')
    if not cmakelists_path or not os.path.isfile(cmakelists_path):
        print("CMake Error: The source directory does not appear to contain CMakeLists.txt.")
        return

    time.sleep(0.5)
    print("-- The CXX compiler identification is GNU 10.2.1")
    time.sleep(0.5)
    print("-- Check for working CXX compiler: /usr/bin/c++ - works")
    time.sleep(0.2)
    print("-- Detecting CXX compiler ABI info - done")
    time.sleep(0.3)
    print("-- Configuring done")
    print("-- Generating done")
    print("-- Build files have been written to: {}".format(os.path.join(sim_cwd, 'build')))
    makefile_path, _ = get_jailed_path(world_root, sim_cwd, 'Makefile')
    if makefile_path:
        with open(makefile_path, 'w') as f:
            f.write("# This is a simulated Makefile.\nall:\n\t@echo Nothing to be done.")

# --- Command Processor ---
def process_command(command_line, character_data, world_root, items_db):
    try:
        parts = shlex.split(command_line)
    except ValueError:
        print("错误: 输入的指令包含未闭合的引号。")
        return character_data
        
    if not parts:
        return character_data

    command = parts[0].lower()
    args = parts[1:]
    
    unlocked_commands = character_data.get('unlocked_commands', [])
    if command not in unlocked_commands:
        print(f"command not found: {command}")
        return character_data

    sim_cwd = character_data.get('pseudo_terminal_cwd', '/')

    if command == 'help':
        handle_help(character_data)
    elif command in ['inventory', 'inv']:
        handle_inventory(character_data, items_db)
    elif command == 'ls':
        handle_ls(args, world_root, sim_cwd)
    elif command == 'cd':
        new_sim_cwd = handle_cd(args, world_root, sim_cwd)
        character_data['pseudo_terminal_cwd'] = new_sim_cwd
    elif command == 'cat':
        handle_cat(args, world_root, sim_cwd)
    elif command == 'whoami':
        print(character_data.get('name', 'user').lower())
    elif command in ['clear', 'cls']:
        os.system('cls' if os.name == 'nt' else 'clear')
        return None
    elif command == 'cmake':
        handle_cmake(args, world_root, sim_cwd)
    
    return character_data

# --- Main Game Loop ---
def main():
    abs_path = os.path.dirname(os.path.abspath(__file__))
    character_file_path = os.path.join(abs_path, CHARACTER_FILE)
    actions_file_path = os.path.join(abs_path, ACTIONS_FILE)
    items_file_path = os.path.join(abs_path, ITEMS_FILE) # New
    world_root = os.path.join(abs_path, 'world')

    character_data = load_json(character_file_path)
    actions = load_json(actions_file_path)
    items_db = load_json(items_file_path) # New
    current_story_file = os.path.join(abs_path, STARTING_STORY_FILE)

    redraw_scene = True
    while True:
        if redraw_scene:
            os.system('cls' if os.name == 'nt' else 'clear')
            story_text, available_choices = parse_story_file(current_story_file)
            typewriter_print(story_text)
            print("\n" + "="*30 + "\n")

            if not available_choices:
                print("...路径似乎在这里中断了。")
                input("(此故事分支尚未完成，按回车键退出。)")
                break

            for i, choice in enumerate(available_choices):
                if "(无法选择)" in choice['text']:
                    print(f"  {i + 1}. {choice['text']}")
                else:
                    print(f"{i + 1}. {choice['text']}")
        
        redraw_scene = False
        prompt = f"[{character_data.get('name', 'user').lower()}@{character_data.get('pseudo_terminal_cwd', '/')}]> "
        player_input = input(prompt)

        try:
            choice_index = int(player_input) - 1
            if 0 <= choice_index < len(available_choices):
                chosen_choice = available_choices[choice_index]
                if "(无法选择)" in chosen_choice['text']:
                    input("这个选项当前无法选择。按回车键重试。")
                    redraw_scene = True
                    continue
                
                chosen_action_name = chosen_choice['action']
                if chosen_action_name in actions:
                    action_details = actions[chosen_action_name]
                    payload = action_details.get('payload', {})
                    
                    action_type = action_details.get('type')
                    if action_type == 'location_change':
                        character_data['location'] = payload.get('new_location')
                        current_story_file = os.path.join(abs_path, payload.get('story_file'))
                    elif action_type == 'unlock_command':
                        cmd_to_unlock = payload.get('command')
                        if cmd_to_unlock and cmd_to_unlock not in character_data['unlocked_commands']:
                            character_data['unlocked_commands'].append(cmd_to_unlock)
                            typewriter_print(f"\n[新指令已解锁: {cmd_to_unlock}]\n")
                            time.sleep(1.5)
                    else: 
                        current_story_file = os.path.join(abs_path, payload.get('story_file'))

                    save_json(character_file_path, character_data)
                    redraw_scene = True
                else:
                    input(f"错误: 动作 '{chosen_action_name}' 未定义。按回车键。")
                    redraw_scene = True
            else:
                input("无效的选择。按回车键重试。")
                redraw_scene = True

        except ValueError:
            updated_data = process_command(player_input, character_data, world_root, items_db)
            if updated_data is None:
                redraw_scene = True
            else:
                character_data = updated_data
                save_json(character_file_path, character_data)
                print()

if __name__ == "__main__":
    main()
