import os
import shlex
import time

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
        print("错误: 输入的指令包含未闭合的引号。" )
        return character_data, False
        
    if not parts:
        return character_data, False

    command = parts[0].lower()
    args = parts[1:]
    
    unlocked_commands = character_data.get('unlocked_commands', [])
    if command not in unlocked_commands:
        print(f"command not found: {command}")
        return character_data, False

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
        return character_data, True
    elif command == 'cmake':
        handle_cmake(args, world_root, sim_cwd)
    
    return character_data, False
