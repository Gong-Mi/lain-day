import os
import shlex
import time
import json
import re

from .game_logic import execute_action

def check_condition(character_data, condition):
    if not condition or not isinstance(condition, dict):
        return True # No condition means always show
    
    flag_name = condition.get('flag')
    required_value = condition.get('value')
    
    # Special check for False, as get() with a default can be tricky
    actual_value = character_data.get('flags', {}).get(flag_name, not required_value)

    return actual_value == required_value

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
    if "arls" in unlocked: print("  arls              - (AR) 扫描当前环境")
    if "examine" in unlocked: print("  examine <ID> [子ID] - 查看场景中物品的详细信息")
    if "cd" in unlocked: print("  cd <路径>         - 切换目录")
    if "cat" in unlocked: print("  cat <文件名>      - 查看文件内容")
    if "whoami" in unlocked: print("  whoami            - 显示当前用户")
    if "clear" in unlocked or "cls" in unlocked: print("  clear/cls         - 清空屏幕")
    if "cmake" in unlocked: print("  cmake .           - (模拟) 尝试构建当前目录的项目")
    if "driver" in unlocked: print("  driver            - 管理驱动 (list, load, unload)")
    if "network" in unlocked: print("  network           - 查看网络状态 (status)")
    if "mail" in unlocked: print("  mail              - 邮件客户端 (list, read, reply, delete)")
    return None # No action to execute

def handle_inventory(character_data, items_db):
    inventory = character_data.get("inventory", {})
    if not inventory:
        print("你的物品栏是空的。")
        return None
    
    print("物品栏:")
    for item_id, quantity in inventory.items():
        item = items_db.get(item_id)
        if item:
            print(f"  - {item.get('name', item_id)} (x{quantity}): {item.get('description', '')}")
        else:
            print(f"  - {item_id} (x{quantity}) (未知物品)")
    return None

def handle_examine(args, character_data, world_map):
    if not args:
        print("用法: examine <物品ID> [子物品ID]")
        return None

    target_id = args[0]
    sub_target_id = args[1] if len(args) > 1 else None
    current_location_id = character_data.get('location')
    location_data = world_map.get(current_location_id, {})
    points_of_interest = location_data.get('points_of_interest', [])

    for poi in points_of_interest:
        if poi.get('id') == target_id:
            if sub_target_id:
                sub_items = poi.get('sub_items', [])
                for sub_item in sub_items:
                    if sub_item.get('id') == sub_target_id:
                        if 'action' in sub_item:
                            return sub_item['action']
                        details = sub_item.get('details')
                        if details:
                            print(details)
                        else:
                            print(f"你仔细看了看 {sub_item.get('description', sub_target_id)}，但没有发现更多信息。")
                        return None
                print(f"在 {poi.get('description', target_id)} 中没有找到 '{sub_target_id}'。")
                return None

            if 'action' in poi:
                return poi['action']

            sub_items = poi.get('sub_items', [])
            visible_sub_items = [s for s in sub_items if check_condition(character_data, s.get('condition'))]

            if visible_sub_items:
                print(f"在 {poi.get('name', target_id)} 中，你发现了以下物品:")
                for sub_item in visible_sub_items:
                    print(f"  - {sub_item.get('name', sub_item.get('id', '未知'))}")
                print(f"\n你可以使用 'examine {target_id} <子物品ID>' 来查看详情。")
                return None
            else:
                # If there are no visible sub-items, just give the standard description.
                details = poi.get('details')
                if details:
                    print(details)
                else:
                    print(f"你仔细看了看 {poi.get('name', target_id)}，但没有发现更多信息。")
                return None

            details = poi.get('details')
            if details:
                print(details)
            else:
                print(f"你仔细看了看 {poi.get('description', target_id)}，但没有发现更多信息。")
            return None

    print(f"你在这里没有看到 '{target_id}'。")
    return None

def handle_driver(args, character_data):
    drivers = character_data.get('drivers', {})
    if not args:
        print("用法: driver <list|load|unload> [驱动名]")
        return None
    subcommand = args[0].lower()
    if subcommand == 'list':
        if not drivers:
            print("没有可管理的驱动。")
            return None
        print("驱动状态:")
        for name, status in drivers.items():
            print(f"  - {name}: {status}")
    elif subcommand in ['load', 'unload']:
        if len(args) < 2:
            print(f"用法: driver {subcommand} <驱动名>")
            return None
        driver_name = args[1]
        if driver_name not in drivers:
            print(f"错误: 未知的驱动 '{driver_name}'。")
            return None
        new_status = 'loaded' if subcommand == 'load' else 'unloaded'
        if drivers[driver_name] == new_status:
            print(f"驱动 '{driver_name}' 已经处于 {new_status} 状态。")
        else:
            drivers[driver_name] = new_status
            print(f"驱动 '{driver_name}' 已被 {new_status}。")
            character_data['drivers'] = drivers
    else:
        print(f"未知的 driver 子命令: '{subcommand}'。可用: list, load, unload。")
    return None

def handle_network(args, character_data):
    if not args or args[0].lower() != 'status':
        print("用法: network status")
        return None
    net_status = character_data.get('network_status', {})
    protocol = net_status.get('protocols', {})
    scope = net_status.get('scope', '未知')
    active_protocols = [p for p, s in protocol.items() if s == 'on']
    print("当前网络状态:")
    print(f"  连接协议: {', '.join(active_protocols) or '无'}")
    print(f"  网络范围: {scope}")
    return None

def handle_mail(args, character_data, world_root):
    maildir_path, _ = get_jailed_path(world_root, '/home/lain', 'Maildir')
    if not maildir_path or not os.path.isdir(maildir_path):
        print("错误: 找不到邮箱目录 Maildir/")
        return None

    if not args:
        print("用法: mail <list|read|reply|delete> [编号]")
        return None

    subcommand = args[0].lower()

    if subcommand == 'list':
        try:
            files = sorted(os.listdir(maildir_path))
        except OSError:
            print("错误: 无法读取邮箱目录。")
            return None
        
        if not files:
            print("收件箱是空的。")
            return None

        mail_cache = []
        print("收件箱:")
        for i, filename in enumerate(files):
            status_char = ' '
            if ',U' in filename:
                status_char = 'N' # New
            
            full_path = os.path.join(maildir_path, filename)
            try:
                with open(full_path, 'r', encoding='utf-8') as f:
                    from_line = f.readline().strip()
                    subject_line = f.readline().strip()
                    from_match = re.match(r'From:\s*(.*)', from_line)
                    subject_match = re.match(r'Subject:\s*(.*)', subject_line)
                    sender = from_match.group(1) if from_match else '未知发件人'
                    subject = subject_match.group(1) if subject_match else '无主题'
                    print(f"  [{i+1}] [{status_char}] 来自: {sender} - {subject}")
                    mail_cache.append(filename)
            except (IOError, IndexError):
                print(f"  [{i+1}] [-] 无法解析邮件: {filename}")
        
        character_data['_mail_session_cache'] = mail_cache

    elif subcommand == 'read':
        if len(args) < 2:
            print("用法: mail read <编号>")
            return None
        try:
            mail_index = int(args[1]) - 1
            mail_cache = character_data.get('_mail_session_cache', [])
            if not (0 <= mail_index < len(mail_cache)):
                print("错误: 无效的邮件编号。")
                return None
            
            filename = mail_cache[mail_index]
            full_path = os.path.join(maildir_path, filename)

            with open(full_path, 'r', encoding='utf-8') as f:
                print("-"*30)
                print(f.read())
                print("-"*30)

            if ',U' in filename:
                new_filename = filename.replace(',U', ',R')
                os.rename(full_path, os.path.join(maildir_path, new_filename))

        except (ValueError, IndexError):
            print("错误: 无效的邮件编号。请先使用 'mail list'。")

    elif subcommand in ['reply', 'delete']:
        print(f"功能 '{subcommand}' 尚未实现。")

    else:
        print(f"未知的 mail 子命令: '{subcommand}'。可用: list, read, reply, delete。")

    return None

def handle_ls(args, world_root, sim_cwd, character_data, fs_access):
    target_sim_path = args[0] if args else sim_cwd
    real_path, norm_sim_path = get_jailed_path(world_root, sim_cwd, target_sim_path)

    # Permission Check
    # We iterate through all protected paths to see if the target is within one of them.
    for path, perms in fs_access.items():
        # os.path.normpath is used to handle path separators consistently
        if os.path.normpath(norm_sim_path).startswith(os.path.normpath(path)):
            required_level = perms.get("list_level", 99) # Default to a high level if not specified
            current_level = character_data.get("credit_level", 0)
            if current_level < required_level:
                print(f"ls: cannot access '{target_sim_path}': Permission denied")
                return None

    if not real_path or not os.path.isdir(real_path):
        print(f"ls: cannot access '{target_sim_path}': No such file or directory")
        return None
    for item in sorted(os.listdir(real_path)):
        print(item)
    return None

def handle_arls(character_data, world_map):
    current_location_id = character_data.get('location')
    if not current_location_id:
        print("错误: 无法确定当前位置。")
        return None
    location_data = world_map.get(current_location_id)
    if not location_data:
        print(f"错误: 在世界地图中找不到当前位置 '{current_location_id}'。")
        return None
    
    points_of_interest = location_data.get('points_of_interest', [])
    
    # Filter POIs based on conditions
    visible_pois = [p for p in points_of_interest if check_condition(character_data, p.get('condition'))]

    if not visible_pois:
        print("AR扫描未发现任何兴趣点。")
        return None
    
    print("正在启动环境扫描...")
    time.sleep(0.5)
    print("AR视觉增强已激活。")
    time.sleep(0.3)
    print("发现以下兴趣点:\n")
    time.sleep(0.5)
    for poi in visible_pois:
        # When displaying, we need to check sub-item conditions as well
        display_description = poi.get('description', '无描述')
        sub_items = poi.get('sub_items', [])
        visible_sub_items = [s for s in sub_items if check_condition(character_data, s.get('condition'))]
        
        # Modify description for bookshelf to hint at content
        if poi.get('id') == 'bookshelf':
            if visible_sub_items:
                display_description = f"{poi.get('description', '')} 里面似乎有些东西。"
            else:
                display_description = f"{poi.get('description', '')} 看起来没什么特别的了。"

        print("  [ {} ${} ] - {}".format(poi.get('name', '未知'), poi.get('id', ''), display_description))
        time.sleep(0.2)
    return None

def handle_cd(args, world_root, sim_cwd):
    if not args:
        return sim_cwd
    target_dir = args[0]
    real_path, new_sim_path = get_jailed_path(world_root, sim_cwd, target_dir)
    if not real_path or not os.path.isdir(real_path):
        print(f"cd: no such file or directory: {target_dir}")
    else:
        sim_cwd = new_sim_path
    return sim_cwd

def handle_cat(args, world_root, sim_cwd, character_data, fs_access):
    if not args:
        print("cat: missing operand")
        return None
    
    real_path, norm_sim_path = get_jailed_path(world_root, sim_cwd, args[0])

    # Permission Check
    for path, perms in fs_access.items():
        if os.path.normpath(norm_sim_path).startswith(os.path.normpath(path)):
            required_level = perms.get("read_level", 99)
            current_level = character_data.get("credit_level", 0)
            if current_level < required_level:
                print(f"cat: {args[0]}: Permission denied")
                return None

    if not real_path or not os.path.isfile(real_path):
        print(f"cat: {args[0]}: No such file or directory")
        return None
    with open(real_path, 'r', encoding='utf-8') as f:
        print(f.read())
    return None

def handle_cmake(args, world_root, sim_cwd):
    if not args or args[0] != '.':
        print("Usage: cmake .")
        return None
    cmakelists_path, _ = get_jailed_path(world_root, sim_cwd, 'CMakeLists.txt')
    if not cmakelists_path or not os.path.isfile(cmakelists_path):
        print("CMake Error: The source directory does not appear to contain CMakeLists.txt.")
        return None
    time.sleep(0.5)
    print("-- The CXX compiler identification is GNU 10.2.1")
    return None

# --- Command Processor ---
def process_command(command_line, character_data, world_root, items_db, world_map, actions, abs_path, fs_access):
    try:
        parts = shlex.split(command_line)
    except ValueError:
        return character_data, None, False, None
        
    if not parts:
        return character_data, None, False, None

    command = parts[0].lower()
    args = parts[1:]
    
    unlocked_commands = character_data.get('unlocked_commands', [])
    if command not in unlocked_commands:
        print(f"command not found: {command}")
        return character_data, None, False, None

    sim_cwd = character_data.get('pseudo_terminal_cwd', '/')
    action_to_execute = None
    needs_redraw = False
    new_story_file = None

    if command == 'help':
        action_to_execute = handle_help(character_data)
    elif command in ['inventory', 'inv']:
        action_to_execute = handle_inventory(character_data, items_db)
    elif command == 'ls':
        action_to_execute = handle_ls(args, world_root, sim_cwd, character_data, fs_access)
    elif command == 'arls':
        action_to_execute = handle_arls(character_data, world_map)
    elif command == 'examine':
        action_to_execute = handle_examine(args, character_data, world_map)
    elif command == 'driver':
        action_to_execute = handle_driver(args, character_data)
    elif command == 'network':
        action_to_execute = handle_network(args, character_data)
    elif command == 'mail':
        action_to_execute = handle_mail(args, character_data, world_root)
    elif command == 'cd':
        character_data['pseudo_terminal_cwd'] = handle_cd(args, world_root, sim_cwd)
    elif command == 'cat':
        action_to_execute = handle_cat(args, world_root, sim_cwd, character_data, fs_access)
    elif command == 'whoami':
        print(character_data.get('name', 'user').lower())
    elif command in ['clear', 'cls']:
        os.system('cls' if os.name == 'nt' else 'clear')
        needs_redraw = True
    elif command == 'cmake':
        action_to_execute = handle_cmake(args, world_root, sim_cwd)
    
    if action_to_execute:
        if action_to_execute in actions:
            action_details = actions[action_to_execute]
            character_data, new_story_file, needs_redraw, _ = execute_action(
                action_details, character_data, abs_path, items_db, action_name=action_to_execute
            )
        else:
            print(f"错误: 动作 '{action_to_execute}' 未在 actions.json 中定义。")

    return character_data, new_story_file, needs_redraw, action_to_execute
